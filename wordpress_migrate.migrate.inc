<?php

/*
 * Implementation of hook_migrate_api().
 */
function wordpress_migrate_migrate_api() {
  $api = array(
    'api' => 2,
    'wizard classes' => array('WordPressMigrateWizard'),
  );
  return $api;
}

class WordPressMigrateWizard extends MigrateUIWizard {
  protected $filename;

  /**
   * Whether to perform author migration.
   *
   * @var bool
   */
  protected $authorMigration = TRUE;

  /**
   * User ID of the account to assign as author of any orphaned content.
   *
   * @var int
   */
  protected $defaultAuthorUid = 1;

  protected $blogPostType;
  protected $pageType;

  protected $contentValues = array();

  /**
   * Lay out the basic steps of the wizard.
   */
  public function __construct() {
    parent::__construct();
    $this->addStep(t('Overview'), 'overviewForm');
    $this->addStep(t('Upload blog'), 'sourceDataForm');
    $this->addStep(t('Select content to import'), 'contentSelectForm');
    $this->addStep(t('Review'), 'reviewForm');
  }

  /**
   * Identify ourselves.
   *
   * @return string
   */
  public function getSourceName() {
    return t('WordPress');
  }

  /**
   * The first step of the wizard - explain what we're doing, and get the
   * migration group.
   *
   * @return array
   */
  protected function overviewForm(&$form_state) {
    $form['overview'] = array(
      '#prefix' => '<p>',
      '#markup' => t('This wizard supports importing into your Drupal site from ' .
        'a WordPress blog. To be able to use this wizard, you must have the ' .
        'address and credentials of the WordPress blog, or an XML file exported ' .
        'from the blog.'),
      '#suffix' => '</p>',
    );
    $form['description'] = array(
      '#prefix' => '<p>',
      '#markup' => t('You will be led through a series of steps, allowing you to ' .
        'customize what will be imported into Drupal and how it will be mapped. ' .
        'At the end of this process, a set of import steps (called <em>Migrations</em>) '.
        'are generated and you will be left at the Migrate dashboard, from which ' .
        'you can perform the imports, monitor their progress, roll them back, etc.'),
      '#suffix' => '</p>',
    );

    $form['group_name'] = array(
      '#type' => 'textfield',
      '#title' => t('Unique name'),
      '#description' => t('Provide a unique alphanumeric name for this import. It ' .
        'will be used to prefix and group the generated migrations.'),
      '#size' => 16,
      '#maxlength' => 16,
      '#required' => TRUE,
      '#default_value' => $this->groupName,
    );
    return $form;
  }

  /**
  * Make sure the migration group is alphanumeric, and save it.
  *
  * @param array $form_state
  */
  protected function overviewFormValidate(&$form_state) {
    $this->groupName = trim($form_state['values']['group_name']);
    if (!preg_match('|^[[:alnum:]]+$|', $this->groupName)) {
      form_set_error('group_name',
        t('The name for your import, %name, must be alphanumeric.',
          array('%name' => $this->groupName)));
    }
  }

  /**
   * Second step - find out how to get the source data.
   */
  public function sourceDataForm(&$form_state) {
    // Make sure we have a private directory configured
    if (!variable_get('wordpress_migrate_private_path', '') &&
        !variable_get('file_private_path', '')) {
      $message = t('A private file system path must be <a href="@config">configured</a>
        to securely store your uploaded WordPress file.',
        array('@config' => url('admin/config/media/file-system')));
      $form['unconfigured'] = array(
        '#prefix' => '<div>',
        '#markup' => $message,
        '#suffix' => '</div>',
      );
      return $form;
    }

    $form['overview'] = array(
      '#prefix' => '<div>',
      '#markup' => t('WordPress blogs can be imported into Drupal using this form.
        You may either provide the necessary credentials for Drupal to retrieve
        your WordPress blog data directly, or you may export the blog yourself
        and upload the exported WXR file.'),
      '#suffix' => '</div>',
    );

    if (module_exists('media') && !module_exists('migrate_extras')) {
      $form['need_extras'] = array(
        '#prefix' => '<div>',
        '#markup' => '<p>' . t('You have the <a href="@media">Media module</a> enabled - to
          take advantage of Media features, you need to also install and enable the
          <a href="@extras">Migrate Extras module</a>.' . '</p>',
          array('@media' => url('http://drupal.org/project/media'),
            '@extras' => url('http://drupal.org/project/migrate_extras'))),
        '#suffix' => '</div>',
      );
    }

    $form['source']['source_select'] = array(
      '#type' => 'radios',
      '#options' => array(0 => t('Import from a file'), 1 => t('Import from a URL')),
      '#default_value' => 0,
      '#attributes' => array('class' => array('container-inline')),
    );

    $form['#attributes'] = array('enctype' => 'multipart/form-data');
    $form['source']['wxr_file'] = array(
      '#type' => 'file',
      '#title' => t('WordPress exported file (WXR)'),
      '#description' => t('Select an exported WordPress file. Maximum file size is !size.',
        array('!size' => format_size(file_upload_max_size()))),
      '#states' => array(
        'visible' => array(
          ':input[name="source_select"]' => array('value' => 0),
        ),
      ),
    );

    $form['source']['credentials']['domain'] = array(
      '#type' => 'textfield',
      '#title' => t('Blog URL'),
      '#description' => t('Enter the URL of the blog to import (e.g. example.wordpress.com).'),
      '#field_prefix' => 'http://',
      '#states' => array(
        'visible' => array(
          ':input[name="source_select"]' => array('value' => 1),
        ),
      ),
    );

    $form['source']['credentials']['username'] = array(
      '#type' => 'textfield',
      '#title' => t('Blog username'),
      '#description' => t(''),
      '#states' => array(
        'visible' => array(
          ':input[name="source_select"]' => array('value' => 1),
        ),
      ),
    );

    $form['source']['credentials']['password'] = array(
      '#type' => 'password',
      '#title' => t('Blog password'),
      '#description' => t(''),
      '#states' => array(
        'visible' => array(
          ':input[name="source_select"]' => array('value' => 1),
        ),
      ),
    );
    return $form;
  }

  /**
   * Fetch and preprocess the uploaded WXR file.
   *
   * @param array $form_state
   */
  protected function sourceDataFormValidate(&$form_state) {
    if ($_FILES['files']['error']['wxr_file'] && empty($form_state['values']['domain'])) {
      form_set_error('wxr_file', t('The file could not be uploaded, most likely
        because the file size exceeds the configured limit of !filesize',
        array('!filesize' => format_size(file_upload_max_size()))));
      return;
    }
    $directory = 'wordpress://';
    if (!file_prepare_directory($directory, FILE_CREATE_DIRECTORY | FILE_MODIFY_PERMISSIONS)) {
      form_set_error('wxr_file',
        t('Could not prepare directory %directory - make sure your Drupal files directory exists and is writable',
          array('%directory' => $directory)));
      return;
    }

    $tmpfile = $_FILES['files']['tmp_name']['wxr_file'];
    // Note that for either path we take here, $tmpfile will be the name of
    // the uploaded/retrieved file, and $destination will be the name of the
    // desired final destination.
    if ($tmpfile) {
      // Handle uploaded file
      $filename = $_FILES['files']['name']['wxr_file'];
      $this->filename = $directory . str_replace(' ', '%20', $filename);
    }
    else {
      // Export the WordPress blog directly
      $domain = preg_replace('/http[s]?\:\/\//', '', $form_state['values']['domain']);

      // Login to the WordPress site
      $wordpress_version = 3;
      $login_url = 'http://' . $domain . '/wp-login.php';
      if (!($handle = fopen($login_url, 'r'))) {
        $wordpress_version = 2;
        $login_url = 'http://' . $domain . '/blog/wp-login.php';
        $handle = fopen($login_url, 'r');
      }
      if (!$handle) {
        form_set_error('credentials][domain', t('Could not find login page for !domain',
            array('!domain' => $domain)));
      }
      fclose($handle);

      // Use a temp file for cookies.
      $cookie_file = file_directory_temp() . '/wpimport-cookie.txt';

      $username = $form_state['values']['username'];
      $password = $form_state['values']['password'];
      $ch = curl_init();
      curl_setopt($ch, CURLOPT_URL, $login_url);
      curl_setopt($ch, CURLOPT_HEADER, 1);
      curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
      curl_setopt($ch, CURLOPT_USERAGENT, 'Importer');
      curl_setopt($ch, CURLOPT_COOKIESESSION, 1);
      curl_setopt($ch, CURLOPT_COOKIEJAR, $cookie_file);
      curl_setopt($ch, CURLOPT_COOKIEFILE, $cookie_file);
      curl_setopt($ch, CURLOPT_POST, 1);
      curl_setopt($ch, CURLOPT_POSTFIELDS, "log=$username&pwd=$password&testcookie=1");
      $login_result = curl_exec($ch);
      curl_close($ch);

      // Login successful? Grab the export
      if ((strpos($login_result, 'Set-Cookie: wordpress_logged_in')) || (strpos($login_result, 'Set-Cookie: wordpressuser_'))) {
        $filename = $domain . '.xml';
        // The "domain" may have included a subdirectory - flatten things out
        $this->filename = $directory . '/' . str_replace('/', '_', $filename);
        $tmpfile = tempnam(sys_get_temp_dir(), 'wp_');
        $export_url = 'http://' . $domain;
        if ($wordpress_version == 2) {
          $export_url .= '/blog';
        }
        $export_url .= '/wp-admin/export.php?mm_start=all&mm_end=all&author=all' .
          '&export_taxonomy[category]=0&export_taxonomy[post_tag]=0&export_post_type=all' .
          '&export_post_status=all&download=true';
        $fp = fopen($tmpfile, 'w');
        if ($fp) {
          $ch = curl_init();
          curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
          curl_setopt($ch, CURLOPT_FILE, $fp);
          curl_setopt($ch, CURLOPT_COOKIEFILE, $cookie_file);
          curl_setopt($ch, CURLOPT_URL, $export_url);
          curl_exec($ch);
          curl_close($ch);
          fclose($fp);
          unlink($cookie_file);
        }
        else {
          form_set_error('credentials][domain', t('Could not create destination file !filename',
            array('!filename' => $tmpfile)));
          return;
        }
      }
      else {
        form_set_error('credentials][domain', t('Could not login at !login_url',
          array('!login_url' => $login_url)));
        return;
      }
    }

    WordPressBlog::preprocessFile($tmpfile, $this->filename);

    // Only include the author step if we have author data (which was introduced
    // in WXR 1.1)
    $blog = wordpress_migrate_blog($this->filename);
    if ($blog->getWxrVersion() != '1.0') {
      $this->addStep(t('Authors'), 'authorForm', $this->currentStep);
    }
    else {
      $this->authorMigration = FALSE;
      global $user;
      $this->defaultAuthorUid = $user->uid;
    }
  }

  /**
   * Present information and options around user migration.
   */
  protected function authorForm(&$form_state) {
    $form['overview'] = array(
      '#prefix' => '<p>',
      '#markup' => t('User accounts for authors in the WordPress blog may be
        imported to Drupal. Note that their passwords are not migrated - they
        must be reset after import.'),
      '#suffix' => '</p>',
    );

    $form['do_migration'] = array(
      '#type' => 'radios',
      '#title' => t('Create new users for existing WordPress content authors?'),
      '#options' => array(1 => t('Yes'), 0 => t('No')),
      '#default_value' => 1,
    );

    global $user;
    $form['default_author'] = array(
      '#type' => 'textfield',
      '#title' => t('Default content author:'),
      '#default_value' => $user->name,
      '#autocomplete_path' => 'user/autocomplete',
      '#states' => array(
        'invisible' => array(
          'input[name="do_migration"]' => array('value' => 1),
        ),
      ),
    );

    return $form;
  }

  /**
   * Prepare for registration of any user-related migrations.
   *
   * @param array $form_state
   */
  protected function authorFormValidate(&$form_state) {
    if ($form_state['values']['do_migration']) {
      $this->authorMigration = TRUE;
    }
    else {
      $this->authorMigration = FALSE;
    }
    $account = user_load_by_name($form_state['values']['default_author']);
    $this->defaultAuthorUid = $account->uid;
  }

  protected function contentSelectForm(&$form_state) {
    // Get destination node type(s)
    $node_types = node_type_get_types();
    $options = array('' => t('Do not import'));
    foreach ($node_types as $node_type => $info) {
      $options[$node_type] = $info->name;
    }

    if (isset($options['blog'])) {
      $default = 'blog';
    }
    else {
      $default = '';
    }

    $form['blog_post_type'] = array(
      '#type' => 'select',
      '#title' => t('Import WordPress blog posts as'),
      '#default_value' => $default,
      '#options' => $options,
      '#description' => t(''),
    );

    if (isset($options['page'])) {
      $default = 'page';
    }
    else {
      $default = '';
    }

    $form['page_type'] = array(
      '#type' => 'select',
      '#title' => t('Import WordPress pages as'),
      '#default_value' => $default,
      '#options' => $options,
      '#description' => t(''),
    );
    return $form;
  }

  protected function contentSelectFormValidate(&$form_state) {
    $this->contentValues['page']['page_type'] = $form_state['values']['page_type'];
    if ($this->contentValues['page']['page_type']) {
      $this->addStep(t('Pages'), 'contentPageForm', $this->currentStep);
    }
    $this->contentValues['blog']['post_type'] = $form_state['values']['blog_post_type'];
    if ($this->contentValues['blog']['post_type']) {
      $this->addStep(t('Blog posts'), 'contentBlogPostForm', $this->currentStep);
    }
  }

  protected function contentBlogPostForm(&$form_state) {
    return $this->contentForm($form_state, $this->contentValues['blog']['post_type']);
  }

  protected function contentPageForm(&$form_state) {
    return $this->contentForm($form_state, $this->contentValues['page']['page_type']);
  }

  /**
   * Form for mapping source content (node) types to destination types.
   */
  protected function contentForm(&$form_state, $selected_post_type) {
    $form = array();

    $vocabs = $this->vocabularies($selected_post_type);
    if (!empty($vocabs)) {
      $options = array('' => t('Do not import'));
      foreach ($vocabs as $machine_name => $name) {
        $options[$machine_name] = $name;
      }

      // If a vocabulary called "tags" exists on the site, default to it.
      $tags_default = (isset($options['tags']) ? 'tags' : '');
      $form['tag_vocabulary'] = array(
        '#type' => 'select',
        '#title' => t('Convert WordPress tags to the vocabulary'),
        '#default_value' => variable_get('wordpress_migrate_tag_vocabulary', $tags_default),
        '#options' => $options,
      );

      $form['category_vocabulary'] = array(
        '#type' => 'select',
        '#title' => t('Convert WordPress categories to the vocabulary'),
        '#default_value' => variable_get('wordpress_migrate_category_vocabulary', ''),
        '#options' => $options,
      );
    }

    $file_fields = array('' => t('Do not import'));
    $file_fields += $this->fileFields($selected_post_type, 'file')
      + $this->fileFields($selected_post_type, 'media');
    $file_image_fields = $file_fields +
                         $this->fileFields($selected_post_type, 'image');
    if (count($file_image_fields) > 1) {
      $form['attachment_field'] = array(
        '#type' => 'select',
        '#title' => t('Field for attachments (including images)'),
        '#default_value' => '',
        '#options' => $file_image_fields,
        '#description' => t(''),
        '#states' => array(
          'visible' => array(
            'input[name="destination_type"]' => array('value' => 'blog'),
          ),
        ),

      );
    }
    else {
      $form['attachment_field'] = array(
          '#type' => 'value',
          '#value' => '',
        );
    }
    if (count($file_fields) > 1) {
      $form['podcast_field'] = array(
        '#type' => 'select',
        '#title' => t('Field for Blubrry PowerPress podcasts'),
        '#default_value' => '',
        '#options' => $file_fields,
        '#description' => t(''),
      );
    }
    else {
      $form['podcast_field'] = array(
        '#type' => 'value',
        '#value' => '',
      );
    }

    $options = array();
    foreach (filter_formats() as $format_id => $format) {
      $options[$format_id] = $format->name;
    }
    $form['text_format'] = array(
      '#type' => 'select',
      '#title' => t('Default format for text fields'),
      '#default_value' => 'filtered_html',
      '#options' => $options,
      '#description' => t(''),
    );
    $form['text_format_comment'] = array(
      '#type' => 'select',
      '#title' => t('Default format for comment text fields'),
      '#default_value' => 'filtered_html',
      '#options' => $options,
      '#description' => t(''),
    );

    if (module_exists('path')) {
      $options = array(
        0 => t('Do not set path aliases'),
        1 => t('Set path aliases to their original WordPress values'),
      );
      $attributes = array();
      $default_value = 1;
      if (module_exists('pathauto')) {
        $options[2] = t('Have pathauto generate new aliases');
        if (!module_exists('migrate_extras')) {
          $form['pathauto_message'] = array(
            '#prefix' => '<p>',
            '#markup' => t('<strong>To be able to change the <em>Path alias handling</em>, ' .
              'you must have the <a href="@extras">Migrate Extras module</a> installed and enabled.</strong>',
                 array('@extras' => url('http://drupal.org/project/migrate_extras'))),
            '#suffix' => '</p>',
          );
          $attributes['disabled'] = TRUE;
          $default_value = 2;
        }
      }
      $form['path_action'] = array(
        '#type' => 'radios',
        '#title' => t('Path alias handling'),
        '#description' => t('Select how path aliases for imported nodes will be set.'),
        '#options' => $options,
        '#default_value' => $default_value,
        '#attributes' => $attributes,
      );
    }

    if (module_exists('redirect')) {
      if (class_exists('MigrateRedirectEntityHandler')) {
        $form['generate_redirects'] = array(
          '#type' => 'checkbox',
          '#title' => t('Generate redirects'),
          '#description' => t('If this box is checked, redirects will be set up from
            the former WordPress blog URLs to the new URLs on your Drupal site'),
          '#default_value' => FALSE,
        );
      }
      else {
        $form['generate_redirects'] = array(
          '#type' => 'value',
          '#value' => 0,
        );
        $form['generate_redirects_info'] = array(
          '#prefix' => '<p>',
          '#markup' => t('You have the Redirect module enabled. To be able to '
            . 'generate redirects for your imported WordPress content, you need '
            . 'to <a href="@link">patch Redirect</a>.',
            array('@link' => 'http://drupal.org/node/1116408#comment-6040494')),
          '#suffix' => '</p>',
        );
      }
    }

    return $form;
  }

  /**
   * Save the selected type mappings, as an array keyed by source type and
   * containing the destination type (bundle).
   */
  protected function contentBlogPostFormValidate(&$form_state) {
    $this->contentValues['blog'] += $form_state['values'];
  }

  /**
   * Save the selected type mappings, as an array keyed by source type and
   * containing the destination type (bundle).
   */
  protected function contentPageFormValidate(&$form_state) {
    $this->contentValues['page'] += $form_state['values'];
  }

  /**
   * Show the user what they've chosen to migrate, and give them one last chance
   * to say yay or nay.
   */
  protected function reviewForm(&$form_state) {
    $message = t('<p>Please review your migration configuration. When you submit this
      form, migration processes will be created and you will be left at the
      migration dashboard.</p>');

    $form['description'] = array(
      '#prefix' => '<div>',
      '#markup' => $message,
      '#suffix' => '</div>',
    );
    $migrations = '';
    $account = user_load($this->defaultAuthorUid);
    if ($this->authorMigration) {
      $migrations .= t('<li>WordPress authors will be imported.</li>');
    }
    else {
      $migrations .= t('<li>WordPress authors will not be imported. The default ' .
        'author for any content whose author can\'t be identified will be %default.</li>',
                       array('%default' => $account->name));
    }
    $arguments = array(
      'default_author_uid' => $this->defaultAuthorUid,
      'import_users' => $this->authorMigration,
    );
    $this->addMigration('Author', 'WordPressAuthor', $arguments);

    if ($this->contentValues['blog']['post_type']) {
      $migrations .= t('<li>WordPress blog posts will be imported to %type nodes.</li>',
        array('%type' => $this->contentValues['blog']['post_type']));
      $this->addMigration('BlogPost', 'WordPressBlogEntry', $this->contentValues['blog']);
    }
    else {
      $migrations .= t('<li>WordPress blog posts will not be imported.</li>');
    }
    if ($this->contentValues['page']['page_type']) {
      $migrations .= t('<li>WordPress pages will be imported to %type nodes.</li>',
        array('%type' => $this->contentValues['page']['page_type']));
      $this->addMigration('Page', 'WordPressPage', $this->contentValues['page']);
    }
    else {
      $migrations .= t('<li>WordPress pages will not be imported.</li>');
    }

    $form['migration_list'] = array(
      '#prefix' => '<ol>',
      '#markup' => $migrations,
      '#suffix' => '</ol>',
    );

    return $form;
  }
  /**
   * Return a list of vocabularies attached to a given node type (bundle).
   *
   * @param $node_type
   * @return array
   */
  function vocabularies($node_type) {
    $fields = field_info_instances('node', $node_type);
    $term_fields = array();
    foreach ($fields as $field_name => $instance_info) {
      $field_info = field_info_field($field_name);
      if ($field_info['type'] == 'taxonomy_term_reference') {
        $term_fields[$field_info['settings']['allowed_values'][0]['vocabulary']] =
          $instance_info['label'];
      }
    }
    return $term_fields;
  }

  /**
   * Return a list of file fields attached to a given node type (bundle).
   *
   * @param $node_type
   * @param $file_type
   * @return array
   */
  function fileFields($node_type, $file_type) {
    $fields = field_info_instances('node', $node_type);
    $file_fields = array();
    foreach ($fields as $field_name => $instance_info) {
      $field_info = field_info_field($field_name);
      if ($field_info['type'] == $file_type) {
        $file_fields[$field_name] = $instance_info['label'];
      }
    }
    return $file_fields;
  }

  /**
   * Record all the information necessary to register a migration when this is
   * all over.
   *
   * @param string $machine_name
   *  Machine name for the migration class.
   * @param string $class_name
   *  Name of the Migration class to instantiate.
   * @param array $arguments
   *  Further information configuring the migration.
   */
  protected function addMigration($machine_name, $class_name, $arguments) {
    $arguments += array(
      'filename' => $this->filename,
    );
    // prepend group top machine name
    parent::addMigration($machine_name, $class_name, $arguments);
  }
}
