<?php

/**
 * @file
 *
 * Web UI for migrating WordPress blogs to Drupal.
 */

/**
 * Menu callback: Returns a page for importing a WordPress blog into Drupal.
 */
function wordpress_migrate_import() {
  drupal_set_title(t('WordPress migration'));
  return drupal_get_form('wordpress_migrate_import_form');
}

/**
 * Form for specifying where to obtain the WordPress content.
 */
function wordpress_migrate_import_form($form, &$form_state) {
  // If an uploaded file exceeds post_max_size, the validate and submit functions
  // never get called (as they will if upload_max_filesize is exceeded but
  // post_max_size isn't). We can detect this case by the presence of an error.
  if ($error = error_get_last()) {
    drupal_set_message(t('File upload failed: !error. Please make sure the file
      you\'re trying to upload is not larger than !limit',
      array('!error' => $error['message'], '!limit' => format_size(file_upload_max_size()))));
  }

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
      '#markup' => t('You have the <a href="@media">Media module</a> enabled - to
        take advantage of Media features, you need to also install and enable the
        <a href="@extras">Migrate Extras module</a>.',
        array('@media' => url('http://drupal.org/project/media'),
          '@extras' => url('http://drupal.org/project/migrate_extras'))),
      '#suffix' => '</div>',
    );
  }

  $form['source'] = array(
    '#type' => 'fieldset',
    '#title' => t('Import source'),
  );
  $form['source']['source_select'] = array(
    '#type' => 'radios',
    '#options' => array(t('Import from a file'), t('Import from a URL')),
    '#default_value' => 0,
    '#attributes' => array('class' => array('container-inline')),
  );

  $form['#attributes'] = array('enctype' => 'multipart/form-data');
  $form['source']['wxr_file'] = array(
    '#type' => 'file',
    '#title' => t('WordPress exported (WXR) file to import into Drupal'),
    '#description' => t('If you have exported your WordPress blog to your local filesystem,
      choose the downloaded file here. The largest file size you can import is !size',
      array('!size' => format_size(file_upload_max_size()))),
    '#states' => array(
      'visible' => array(
        ':input[name="source_select"]' => array('value' => 0),
      ),
    ),
  );

  $form['source']['credentials'] = array(
    '#type' => 'fieldset',
    '#title' => t('WordPress credentials'),
    '#states' => array(
      'visible' => array(
        ':input[name="source_select"]' => array('value' => 1),
      ),
    ),
  );

  $form['source']['credentials']['domain'] = array(
    '#type' => 'textfield',
    '#title' => t('Domain of your blog'),
    '#description' => t('Enter the URL of the blog to import (e.g., example.wordpress.com)'),
  );

  $form['source']['credentials']['username'] = array(
    '#type' => 'textfield',
    '#title' => t('Username of your WordPress account'),
    '#description' => t(''),
  );

  $form['source']['credentials']['password'] = array(
    '#type' => 'password',
    '#title' => t('Password to your WordPress account'),
    '#description' => t(''),
  );

  // If they haven't yet configured their import, open up the field set. Or, if
  // we're being reloaded by AJAX, also keep it open.
  $post_type = variable_get('wordpress_migrate_post_type', '');
  if (isset($form_state['values']) || !$post_type) {
    $collapsed = FALSE;
  }
  else {
    $collapsed = TRUE;
  }

  $form['settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Import settings'),
    '#collapsible' => TRUE,
    '#collapsed' => $collapsed,
  );

  // Select destination node type(s)
  $node_types = node_type_get_types();
  $options = array();
  foreach ($node_types as $node_type => $info) {
    $options[$node_type] = $info->name;
  }

  if (isset($options['page'])) {
    $default = 'page';
  }
  else {
    $default = '';
  }

  $form['settings']['wordpress_migrate_page_type'] = array(
    '#type' => 'select',
    '#title' => t('Convert WordPress pages to'),
    '#default_value' => variable_get('wordpress_migrate_page_type', $default),
    '#options' => $options,
    '#description' => t(''),
  );

  if (isset($options['blog'])) {
    $default = 'blog';
  }
  else {
    $default = '';
  }

  $post_type = variable_get('wordpress_migrate_post_type', $default);
  $form['settings']['wordpress_migrate_post_type'] = array(
    '#type' => 'select',
    '#title' => t('Convert WordPress posts to'),
    '#default_value' => $post_type,
    '#options' => $options,
    '#description' => t(''),
    '#ajax' => array(
      'wrapper' => 'edit-settings',
      'callback' => 'wordpress_migrate_post_type_change',
      'method' => 'replace',
    ),
  );

  if (isset($form_state['values'])) {
    $selected_post_type = $form_state['values']['wordpress_migrate_post_type'];
  }
  else {
    $selected_post_type = $post_type;
  }

  $file_fields = array('' => t('Do not import'));
  $file_fields += wordpress_migrate_filefields($selected_post_type, 'file')
    + wordpress_migrate_filefields($selected_post_type, 'media');
  $file_image_fields = $file_fields +
                       wordpress_migrate_filefields($selected_post_type, 'image');
  if (count($file_image_fields) > 1) {
    $form['settings']['wordpress_migrate_attachment_field'] = array(
      '#type' => 'select',
      '#title' => t('Field for attachments'),
      '#default_value' => variable_get('wordpress_migrate_attachment_field', ''),
      '#options' => $file_image_fields,
      '#description' => t(''),
    );
  }
  else {
    $form['settings']['wordpress_migrate_attachment_field'] = array(
        '#type' => 'value',
        '#value' => '',
      );
  }
  if (count($file_fields) > 1) {
    $form['settings']['wordpress_migrate_podcast_field'] = array(
      '#type' => 'select',
      '#title' => t('Field for Blubrry PowerPress podcasts'),
      '#default_value' => variable_get('wordpress_migrate_podcast_field', ''),
      '#options' => $file_fields,
      '#description' => t(''),
    );
  }
  else {
    $form['settings']['wordpress_migrate_podcast_field'] = array(
      '#type' => 'value',
      '#value' => '',
    );
  }

  // Select default text format for bodies etc.
  $options = array();
  foreach (filter_formats() as $format_id => $format) {
    $options[$format_id] = $format->name;
  }
  $form['settings']['wordpress_migrate_text_format'] = array(
    '#type' => 'select',
    '#title' => t('Default format for text fields'),
    '#default_value' => variable_get('wordpress_migrate_text_format', 'filtered_html'),
    '#options' => $options,
    '#description' => t(''),
  );
  $form['settings']['wordpress_migrate_text_format_comment'] = array(
    '#type' => 'select',
    '#title' => t('Default format for comment text fields'),
    '#default_value' => variable_get('wordpress_migrate_text_format_comment', 'filtered_html'),
    '#options' => $options,
    '#description' => t(''),
  );

  // TODO: Select user to own blog

  // Select vocabularies for categories and tags
  if (module_exists('taxonomy')) {
    $vocabs = wordpress_migrate_vocabularies($selected_post_type);
    $options = array('' => t('Do not import'));
    foreach ($vocabs as $machine_name => $name) {
      $options[$machine_name] = $name;
    }

    $form['settings']['wordpress_migrate_tag_vocabulary'] = array(
      '#type' => 'select',
      '#title' => t('Convert WordPress tags to the vocabulary'),
      '#default_value' => variable_get('wordpress_migrate_tag_vocabulary', ''),
      '#options' => $options,
    );

    $form['settings']['wordpress_migrate_category_vocabulary'] = array(
      '#type' => 'select',
      '#title' => t('Convert WordPress categories to the vocabulary'),
      '#default_value' => variable_get('wordpress_migrate_category_vocabulary', ''),
      '#options' => $options,
    );
  }

  if (module_exists('path')) {
    $options = array(
      0 => t('Do not set path aliases'),
      1 => t('Set path aliases to their original WordPress values'),
    );
    $attributes = array();
    $default_value = variable_get('wordpress_migrate_path_action', 1);
    if (module_exists('pathauto')) {
      $options[2] = t('Have pathauto generate new aliases');
      if (!module_exists('migrate_extras')) {
        $form['settings']['wordpress_migrate_pathauto_message'] = array(
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
    $form['settings']['wordpress_migrate_path_action'] = array(
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
      $form['settings']['wordpress_migrate_generate_redirects'] = array(
        '#type' => 'checkbox',
        '#title' => t('Generate redirects'),
        '#description' => t('If this box is checked, redirects will be set up from
          the former WordPress blog URLs to the new URLs on your Drupal site'),
        '#default_value' => variable_get('wordpress_migrate_generate_redirects', FALSE),
      );
    }
    else {
      $form['settings']['wordpress_migrate_generate_redirects'] = array(
        '#type' => 'value',
        '#value' => 0,
      );
      $form['settings']['wordpress_migrate_generate_redirects_info'] = array(
        '#prefix' => '<p>',
        '#markup' => t('You have the Redirect module enabled. To be able to '
          . 'generate redirects for your imported WordPress content, you need '
          . 'to <a href="@link">patch Redirect</a>.',
          array('@link' => 'http://drupal.org/node/1116408#comment-6040494')),
        '#suffix' => '</p>',
      );
    }
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import WordPress blog'),
  );

  return $form;
}

/**
 * Validate callback for the WordPress import form.
 */
function wordpress_migrate_import_form_validate($form, &$form_state) {
  $values = $form_state['values'];
  if (module_exists('redirect')
      && $values['wordpress_migrate_path_action'] == 1
      && $values['wordpress_migrate_generate_redirects'] == 1) {
    form_set_error('wordpress_migrate_generate_redirects',
      t('It is not meaningful to set redirects from the WordPress aliases '
        . 'when they are also setting the Drupal aliases. Please either make '
        . 'another <strong>Path alias handling</strong> choice, or turn off '
        . '<strong>Generate redirects</strong>.'));
  }
}

/**
 * Submit callback for the WordPress import form.
 */
function wordpress_migrate_import_form_submit($form, &$form_state) {
  $values = $form_state['values'];

  // First, save all the settings.
  variable_set('wordpress_migrate_post_type', $values['wordpress_migrate_post_type']);
  variable_set('wordpress_migrate_attachment_field', $values['wordpress_migrate_attachment_field']);
  variable_set('wordpress_migrate_podcast_field', $values['wordpress_migrate_podcast_field']);
  variable_set('wordpress_migrate_text_format', $values['wordpress_migrate_text_format']);
  variable_set('wordpress_migrate_text_format_comment', $values['wordpress_migrate_text_format_comment']);
  variable_set('wordpress_migrate_page_type', $values['wordpress_migrate_page_type']);
  if (module_exists('taxonomy')) {
    variable_set('wordpress_migrate_tag_vocabulary', $values['wordpress_migrate_tag_vocabulary']);
    variable_set('wordpress_migrate_category_vocabulary', $values['wordpress_migrate_category_vocabulary']);
  }
  if (module_exists('path')) {
    variable_set('wordpress_migrate_path_action', $values['wordpress_migrate_path_action']);
  }
  if (module_exists('redirect')) {
    variable_set('wordpress_migrate_generate_redirects', $values['wordpress_migrate_generate_redirects']);
  }

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
    $destination = $directory . str_replace(' ', '%20', $filename);
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
    if (strpos($login_result, 'Set-Cookie: wordpress_logged_in')) {
      $filename = $domain . '.xml';
      // The "domain" may have included a subdirectory - flatten things out
      $destination = $directory . '/' . str_replace('/', '_', $filename);
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

  // Cleanup some stuff in the process of moving the file to its final
  // destination
  $temp_handle = fopen($tmpfile, 'r');
  $dest_handle = fopen($destination, 'w');

  // First, get the header (everything before the <channel> element) to
  // rewrite the namespaces
  $header = '';
  while ($line = fgets($temp_handle)) {
    $header .= $line;
    if (strpos($line, '<channel>') !== FALSE) {
      break;
    }
  }

  // The excerpt namespace is sometimes omitted, stuff it in if necessary
  $excerpt_ns = 'xmlns:excerpt="http://wordpress.org/export/1.0/excerpt/"';
  $excerpt_signature = 'xmlns:excerpt="http://wordpress.org/export/';
  $content_ns = 'xmlns:content="http://purl.org/rss/1.0/modules/content/"';
  if (!strpos($header, $excerpt_signature)) {
    $header = str_replace($content_ns, $excerpt_ns . "\n\t" . $content_ns, $header);
  }
  // Add the Atom namespace, in case it's referenced
  $atom_ns = 'xmlns:atom="http://www.w3.org/2005/Atom"';
  $header = str_replace($content_ns, $atom_ns . "\n\t" . $content_ns, $header);

  // What the hell, throw in iTunes too
  $itunes_ns = 'xmlns:itunes="http://www.itunes.com/dtds/podcast-1.0.dtd"';
  $header = str_replace($content_ns, $itunes_ns . "\n\t" . $content_ns, $header);

  fputs($dest_handle, $header);

  // Now, do some line-by-line fix-ups fix unencoded ampersands and bogus characters on a line-by-line basis
  while ($line = fgets($temp_handle)) {
    // Handle unencoded ampersands
    $line = preg_replace('/&(?![\w\d#]+;)/', '&amp;', $line);
    // Remove control characters (the regex removes the newline, so tack it back on)
    $line = preg_replace('~\p{C}+~u', '', $line) . "\n";
    // WordPress export doesn't properly format embedded CDATA sections - our
    // quick-and-dirty fix is to remove the terminator of the embedded section
    $line = preg_replace('|// \]\]|', '', $line);
    fputs($dest_handle, $line);
  }

  fclose($dest_handle);
  fclose($temp_handle);
  unlink($tmpfile);

  try {
    $blog = wordpress_migrate_blog($destination);
  }
  catch (Exception $e) {
    form_set_error('wxr_file', $e->getMessage());
    file_unmanaged_delete($destination);
    return;
  }

  // Import each migration in order, until done or time is running out
  if (variable_get('wordpress_migrate_import_method', 0) == 0) {
    $operations = array();
    foreach ($blog->migrations() as $migration) {
       $operations[] = array('migrate_ui_batch',
                              array('import', $migration->getMachineName(),
                                    array('unit' => 'items', 'value' => ''), FALSE));
    }

    $batch = array(
      'operations' => $operations,
      'title' => t('WordPress import in progress'),
      'file' => drupal_get_path('module', 'migrate_ui') .
                    '/migrate_ui.pages.inc',
      'init_message' => t('Starting import process'),
      'progress_message' => t(''),
      'error_message' => t('An error occurred. Some or all of the import processing has failed.'),
      'finished' => 'migrate_ui_batch_finish',
    );
    batch_set($batch);
  }
  else {
    $spawned = FALSE;
    foreach ($blog->migrations() as $migration) {
      $result = $migration->processImport();
      if ($result == MigrationBase::RESULT_INCOMPLETE) {
        $drush = variable_get('wordpress_migrate_drush', '');
        if (!$drush) {
          $message = t('The blog was too large to import through the browser - please
              <a href="@config">configure drush</a> so the import process may be
              run in the background.',
            array('@config' => url('admin/content/wordpress')));
          form_set_error('wxr_file', $message);
          break;
        }
        drupal_set_message(t('The blog is too large to completely import immediately -
          the rest of the import will be run in the background and you will receive an email
          when it is complete'));
        $uri = 'http://' . $_SERVER['HTTP_HOST'];
        $log_file = '/tmp/' . $filename . '.import.log';
        $command = "$drush --uri=$uri wordpress-migrate-import --root=" .
                   DRUPAL_ROOT . " $destination >$log_file 2>&1 &";
        exec($command);
        $spawned = TRUE;
        break;
      }
    }
    if (!$spawned) {
      drupal_set_message(t('File %filename successfully uploaded and imported',
        array('%filename' => $filename)));
    }
  }
  $form_state['redirect'] = 'admin/content/wordpress/review';
}

/**
 * Return a list of file fields attached to a given node type (bundle).
 *
 * @param $node_type
 * @param $file_type
 * @return array
 */
function wordpress_migrate_filefields($node_type, $file_type) {
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
 * Return a list of vocabularies attached to a given node type (bundle).
 *
 * @param $node_type
 * @return array
 */
function wordpress_migrate_vocabularies($node_type) {
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

function wordpress_migrate_post_type_change($form, $form_state) {
  return $form['settings'];
}

/**
 * Menu callback: Returns a page for reviewing WordPress migrations.
 */
function wordpress_migrate_review() {
  drupal_set_title(t('WordPress migrations'));
  return drupal_get_form('wordpress_migrate_review_form');
}

/**
 * Form for reviewing WordPress migrations.
 */
function wordpress_migrate_review_form($form, &$form_state) {
  if (isset($form_state['values']['operation']) &&
      ($form_state['values']['operation'] == 'rollback' || $form_state['values']['operation'] == 'clear')) {
    return wordpress_migrate_rollback_confirm($form, $form_state, array_filter($form_state['values']['blogs']));
  }
  $form['overview'] = array(
    '#prefix' => '<p>',
    '#markup' => t('These are the WordPress blogs which you have imported into your
      Drupal site. For each component of the blog, the number of imported items is
      displayed.'),
    '#suffix' => '</p>',
  );

  $header = array(
    'blog_url' => array('data' => t('Blog URL')),
    'status' => array('data' => t('Status')),
    'WordPressAuthor' => array('data' => t('Authors')),
  );
  if (module_exists('taxonomy')) {
    if (variable_get('wordpress_migrate_category_vocabulary', '')) {
      $header += array(
        'WordPressCategory' => array('data' => t('Categories')),
      );
    }
    if (variable_get('wordpress_migrate_tag_vocabulary', '')) {
      $header += array(
        'WordPressCategory' => array('data' => t('Tags')),
      );
    }
  }
  $header += array(
    'WordPressBlogEntry' => array('data' => t('Posts')),
    'WordPressPage' => array('data' => t('Pages')),
    'WordPressAttachment' => array('data' => t('Attachments')),
  );
  if (module_exists('comment')) {
    $header += array(
      'WordPressComment' => array('data' => t('Comments')),
    );
  }
  $rows = array();
  $blogs = call_user_func(array(wordpress_migrate_blog_class(), 'blogs'));
  $author_found = FALSE;
  foreach ($blogs as $blog) {
    $classes = array_flip($blog->migrationClasses());
    $row['blog_url'] = $blog->getBlogUrl();
    $row['status'] = t('Complete');
    foreach ($blog->migrations() as $migration) {
      $class = $classes[get_class($migration)];
      if ($class == 'WordPressAuthor') {
        $author_found = TRUE;
      }
      $row[$class] = $migration->importedCount();
      $status = $migration->getStatus();
      if ($status == MigrationBase::STATUS_IMPORTING) {
        $row['status'] = t('Importing');
      }
      elseif ($status == MigrationBase::STATUS_ROLLING_BACK) {
        $row['status'] = t('Deleting');
      }
    }
    $rows[$blog->getBlogUrl()] = $row;
  }

  if (!$author_found) {
    unset($header['WordPressAuthor']);
  }
  $form['blogs'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $rows,
    '#empty' => t('No WordPress blogs have been migrated into your Drupal site'),
  );

  // Build the 'Update options' form.
  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Update options'),
    '#attributes' => array('class' => array('container-inline')),
  );
  $options = array(
    'rollback' => t('Remove imported content'),
    'clear' => t('Remove migration bookkeeping'),
  );
  $form['options']['description'] = array(
    '#prefix' => '<div>',
    '#markup' => t('The WordPress migration process does considerable bookkeeping,
      tracking how the original WordPress content maps to the imported Drupal content.
      This bookkeeping allows you to easily back out the migration, restoring your
      Drupal site to its initial state; once you are satisfied with the imported
      content, you may remove the bookkeeping overhead.
      <ul><li><strong>Remove imported content</strong> will restore
      your Drupal site to its state before the selected WordPress blogs were
      imported, deleting all imported content as well as the bookkeeping overhead.</li>
      <li><strong>Remove bookkeeping only</strong> will remove the bookkeeping
      overhead, retaining all imported content.</li></ul>'),
    '#postfix' => '</div>',
  );
  $form['options']['operation'] = array(
    '#type' => 'select',
    '#title' => t('Operation'),
    '#title_display' => 'invisible',
    '#options' => $options,
  );
  $form['options']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
    '#validate' => array('wordpress_migrate_review_validate'),
    '#submit' => array('wordpress_migrate_review_submit'),
  );

  return $form;
}

/**
 * Validate callback for the WordPress review form.
 */
function wordpress_migrate_review_validate($form, &$form_state) {
  // Error if there are no items to select.
  if (!is_array($form_state['values']['blogs']) || !count(array_filter($form_state['values']['blogs']))) {
    form_set_error('', t('No items selected.'));
  }
}

/**
 * Submit callback for the WordPress review form.
 */
function wordpress_migrate_review_submit($form, &$form_state) {
  // We need to rebuild the form to go to a second step (confirm blog rollback)
  $form_state['rebuild'] = TRUE;
}

function wordpress_migrate_rollback_confirm($form, &$form_state, $blogs) {
  $operation = $form_state['values']['operation'];

  $form['blogs'] = array('#prefix' => '<ul>', '#suffix' => '</ul>', '#tree' => TRUE);
  // array_filter returns only elements with TRUE values
  foreach ($blogs as $blog_url) {
    $form['blogs'][$blog_url] = array(
      '#type' => 'hidden',
      '#value' => $blog_url,
      '#prefix' => '<li>',
      '#suffix' => check_plain($blog_url) . "</li>\n",
    );
  }
  $form['operation'] = array('#type' => 'hidden', '#value' => $operation);
  $form['#submit'][] = 'wordpress_migrate_rollback_confirm_submit';
  if ($operation == 'rollback') {
    $confirm_question = format_plural(count($blogs),
                                    'Are you sure you want to remove all imported content for this blog?',
                                    'Are you sure you want to remove all imported content for these blogs?');
  }
  else {
    $confirm_question = format_plural(count($blogs),
                                    'Are you sure you want to remove all migration bookkeeping for this blog?',
                                    'Are you sure you want to remove all migration bookkeeping for these blogs?');
  }
  return confirm_form($form,
                    $confirm_question,
                    'admin/content/wordpress/review', t('This action cannot be undone.'),
                    t('Delete'), t('Cancel'));
}

function wordpress_migrate_rollback_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    $operations = array();
    $blogs = array_keys($form_state['values']['blogs']);
    foreach ($blogs as $blog_url) {
      // Rollback migrations for this domain
      $filename = db_select('wordpress_migrate', 'wp')
                   ->fields('wp', array('filename'))
                   ->condition('blog_url', $blog_url)
                   ->execute()
                   ->fetchField();
      $blog = wordpress_migrate_blog($filename);
      $migrations = array_reverse($blog->migrations());
      foreach ($migrations as $migration) {
        // Only rollback content for the rollback operation
        if ($form_state['values']['operation'] == 'rollback') {
          // If not currently idle, stop it before attempting rollback
          if ($migration->getStatus() != MigrationBase::STATUS_IDLE) {
            $migration->stopProcess();
            // Give it a little time to react to the stop request
            $count = 5;
            while ($count && $migration->getStatus() == MigrationBase::STATUS_STOPPING) {
              sleep(2);
              $count--;
            }
            if ($migration->getStatus() == MigrationBase::STATUS_STOPPING) {
              // At this point, assume it's stuck and reset the status so we can continue
              $migration->resetStatus();
            }
          }
          $operations[] = array('migrate_ui_batch',
                                array('rollback', $migration->getMachineName(),
                                      array('unit' => 'items', 'value' => '')));
        }
      }
    }
    $batch = array(
      'operations' => $operations,
      'title' => t('WordPress rollback in progress'),
      'file' => drupal_get_path('module', 'migrate_ui') .
                    '/migrate_ui.pages.inc',
      'init_message' => t('Starting rollback process'),
      'progress_message' => t(''),
      'error_message' => t('An error occurred. Some or all of the rollback processing has failed.'),
      'finished' => 'migrate_ui_batch_finish',
    );
    batch_set($batch);
    $operations = array();
    $operations[] = array('wordpress_migrate_batch_clear', array($blogs));
    $batch = array(
      'operations' => $operations,
      'title' => t('WordPress rollback in progress'),
      'file' => drupal_get_path('module', 'wordpress_migrate') .
                    '/wordpress_migrate.pages.inc',
      'init_message' => t('Clearing housekeeping'),
      'progress_message' => t(''),
      'error_message' => t('An error occurred. Some or all of the rollback processing has failed.'),
      'finished' => 'migrate_ui_batch_finish',
    );
    batch_set($batch);
  }
}

function wordpress_migrate_batch_clear($blogs, &$context) {
  foreach ($blogs as $blog_url) {
    $filename = db_select('wordpress_migrate', 'wp')
                 ->fields('wp', array('filename'))
                 ->condition('blog_url', $blog_url)
                 ->execute()
                 ->fetchField();
    $blog = wordpress_migrate_blog($filename);
    $migrations = $blog->migrations();
    foreach ($migrations as $migration) {
      Migration::deregisterMigration($migration->getMachineName());
    }
    // Clear wordpress_migrate table entry
    db_delete('wordpress_migrate')
      ->condition('blog_url', $blog_url)
      ->execute();

    // Delete WXR file
    file_unmanaged_delete($filename);

    // Delete photo gallery
    if (module_exists('media_gallery')) {
      global $user;
      $blog_title = t("@name's blog", array('@name' => format_username($user)));
      $gallery_nid = db_select('node', 'n')
        ->fields('n', array('nid'))
        ->condition('type', 'media_gallery')
        ->condition('title', $blog_title)
        ->execute()
        ->fetchField();
      if ($gallery_nid) {
        node_delete($gallery_nid);
      }
    }
  }
  $context['finished'] = 1;
  $context['results'][] = format_plural(count($blogs), 'Deleted 1 WordPress blog.',
    'Deleted @count WordPress blogs.');
}
