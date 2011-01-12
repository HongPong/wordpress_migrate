<?php
// $Id$

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
  $scheme = variable_get('wordpress_migrate_scheme', '');
  if (!$scheme) {
    if (user_access(WORDPRESS_MIGRATE_ACCESS_CONFIGURE)) {
      $message = t('Wordpress migration must be <a href="@config">configured</a> before use',
        array('@config' => url('admin/content/wordpress/configure')));
    }
    else {
      $message = t('WordPress migration is not properly configured - please contact
        a site administrator');
    }
    $form['unconfigured'] = array(
      '#prefix' => '<div>',
      '#markup' => $message,
      '#suffix' => '</div>',
    );
    return $form;
  }
  elseif (user_access(WORDPRESS_MIGRATE_ACCESS_CONFIGURE)) {
    $form['configure'] = array(
      '#prefix' => '<div>',
      '#markup' => t('<a href="@config">Configuration</a>',
        array('@config' => url('admin/content/wordpress/configure'))),
      '#suffix' => '</div>',
    );
  }

  $form['overview'] = array(
    '#prefix' => '<div>',
    '#markup' => t('WordPress blogs can be imported into Drupal using this form.
      You may either provide the necessary credentials for Drupal to retrieve
      your WordPress blog data directly, or you may export the blog yourself
      and upload the exported WXR file.'),
    '#suffix' => '</div>',
  );

  $form['credentials'] = array(
    '#type' => 'fieldset',
    '#title' => t('WordPress blog credentials'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  $form['credentials']['description'] = array(
    '#prefix' => '<div>',
    '#markup' => t('To import your blog directly from WordPress, enter your credentials here.'),
    '#suffix' => '</div>',
  );

  $form['credentials']['domain'] = array(
    '#type' => 'textfield',
    '#title' => t('Domain of your blog'),
    '#description' => t('Enter the domain of the blog to import (e.g., example.wordpress.com)'),
  );

  $form['credentials']['username'] = array(
    '#type' => 'textfield',
    '#title' => t('Username of your WordPress account'),
    '#description' => t(''),
  );

  $form['credentials']['password'] = array(
    '#type' => 'password',
    '#title' => t('Password to your WordPress account'),
    '#description' => t(''),
  );

  $form['#attributes'] = array('enctype' => 'multipart/form-data');
  $form['wxr_file'] = array(
    '#type' => 'file',
    '#title' => t('WordPress exported (WXR) file to import into Drupal'),
    '#description' => t('If you have exported your WordPress blog to your local filesystem,
      choose the downloaded file here.')
  );

  global $user;
  $form['notify'] = array(
    '#type' => 'textfield',
    '#title' => t('Email address for notification'),
    '#default_value' => $user->mail,
    '#description' => t('If the blog is too large to be completely imported
      immediately, the import will be run in the background and the specified
      email address will be notified when it is complete.'),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import WordPress blog'),
  );

  return $form;
}

/**
 * Submit callback for the WordPress import form.
 */
function wordpress_migrate_import_form_submit($form, &$form_state) {
  // Get the scheme for saving the WXR file. Note that we default to private, but
  // that might not be available, so check - if the scheme is not available, punt.
  $scheme = variable_get('wordpress_migrate_scheme', 'private');
  $streams = file_get_stream_wrappers(STREAM_WRAPPERS_WRITE);
  if (!isset($streams[$scheme])) {
    if (user_access(WORDPRESS_MIGRATE_ACCESS_CONFIGURE)) {
      $message = t('Scheme %scheme not available - please <a href="@config">configure
          an appropriate scheme</a> for saving the WordPress WXR file.',
        array('%scheme' => $scheme, '@config' => url('admin/content/wordpress/configure')));
    }
    else {
      $message = t('WordPress migration is not properly configured - please contact
        a site administrator');
    }
    form_set_error('wxr_file', $message);
    return;
  }

  $directory = $scheme . '://wordpress';
  if (!file_prepare_directory($directory, FILE_CREATE_DIRECTORY | FILE_MODIFY_PERMISSIONS)) {
    form_set_error('wxr_file', t('Could not prepare directory %directory',
      array('%directory' => $directory)));
    return;
  }

  $tmpfile = $_FILES['files']['tmp_name']['wxr_file'];
  if ($tmpfile) {
    // Handle uploaded file
    $filename = $_FILES['files']['name']['wxr_file'];
    $destination = $directory . '/' . $filename;
    $saved = file_unmanaged_move($tmpfile, $destination, FILE_EXISTS_REPLACE);
    if (!$saved) {
      drupal_set_message(t('Failed to save file to %filename', array('%filename' => $destination)));
    }
  }
  else {
    // Export the WordPress blog directly
    $domain = $form_state['values']['domain'];

    // Login to the WordPress site
    $login_url = 'http://' . $domain . '/wp-login.php';
    $username = $form_state['values']['username'];
    $password = $form_state['values']['password'];
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, $login_url);
    curl_setopt($ch, CURLOPT_HEADER, 1);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
    curl_setopt($ch, CURLOPT_USERAGENT, 'Importer');
    curl_setopt($ch, CURLOPT_COOKIESESSION, 1);
    curl_setopt($ch, CURLOPT_COOKIEJAR, 'cookie.txt');
    curl_setopt($ch, CURLOPT_COOKIEFILE, 'cookie.txt');
    curl_setopt($ch, CURLOPT_POST, 1);
    curl_setopt($ch, CURLOPT_POSTFIELDS, "log=$username&pwd=$password");
    $login_result = curl_exec($ch);
    curl_close($ch);

    // Login successful? Grab the export
    if (strpos($login_result, 'Set-Cookie: wordpress_logged_in_')) {
      $filename = $domain . '.xml';
      $destination = $directory . '/' . $filename;
      $export_url = 'http://' . $domain  . '/wp-admin/export.php?mm_start=all&mm_end=all&author=all' .
        '&export_taxonomy[category]=0&export_taxonomy[post_tag]=0&export_post_type=all' .
        '&export_post_status=all&download=true';
      $fp = fopen($destination, 'w');
      if ($fp) {
        $ch = curl_init();
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
        curl_setopt($ch, CURLOPT_FILE, $fp);
        curl_setopt($ch, CURLOPT_COOKIEFILE, 'cookie.txt');
        curl_setopt($ch, CURLOPT_URL, $export_url);
        curl_exec($ch);
        curl_close($ch);
        fclose($fp);
        $saved = TRUE;
      }
      else {
        form_set_error('credentials][domain', t('Could not create destination file !filename',
          array('!filename' => $destination)));
        $saved = FALSE;
      }
    }
    else {
      form_set_error('credentials][domain', t('Could not login at !login_url',
        array('!login_url' => $login_url)));
      $saved = FALSE;
    }
  }

  if ($saved) {
    // Extract the blog title, which will be used to construct migration machine names
    $xml = simplexml_load_file($destination);
    $title = (string)$xml->channel->title;
    // Keep only alphabetic characters
    // TODO: Use filename if nothing's left
    $title = preg_replace('/[^A-Za-z]/', '', $title);

    // Save info on this blog (including UID of the account performing import)
    // TODO: What to do if this has previously been imported?
    global $user;
    db_merge('wordpress_migrate')
      ->key(array('filename' => $destination))
      ->fields(array('title' => $title))
      ->fields(array('uid' => $user->uid))
      ->execute();

    // Instantiate each WP migration, passing the filename
    $log_file = '/tmp/' . $filename . '.log';
    $migration_classes = WordPressMigration::migrationList();
    $perform_import = TRUE;
    foreach ($migration_classes as $class_name) {
      // Skip tags/categories if there's no destination vocabulary
      if ($class_name == 'WordPressTag' && !variable_get('wordpress_migrate_tag_vocabulary', '') ||
          $class_name == 'WordPressCategory' && !variable_get('wordpress_migrate_category_vocabulary', '')) {
        continue;
      }
      $machine_name = _wordpress_machine_from_title($class_name, $title);
      MigrationBase::registerMigration($class_name, $machine_name, array('filename' => $destination));
      if ($perform_import) {
        $migration = MigrationBase::getInstance($machine_name);
        $result = $migration->processImport();
        if ($result == MigrationBase::RESULT_INCOMPLETE) {
          $drush = variable_get('wordpress_migrate_drush', '');
          if (!$drush) {
            if (user_access(WORDPRESS_MIGRATE_ACCESS_CONFIGURE)) {
              $message = t('The blog was too large to import through the browser - please
                  <a href="@config">configure drush</a> so the import process may be
                  run in the background.',
                array('@config' => url('admin/content/wordpress/configure')));
            }
            else {
              $message = t('The blog was too large to import through the browser - please
                contact a site administrator to properly configure the site for
                background imports.');
            }
            form_set_error('wxr_file', $message);
            $perform_import = FALSE;
          }
          drupal_set_message(t('The blog is too large to completely import immediately -
            the rest of the import will be run in the background and you will receive an email
            when it is complete'));
          $uri = 'http://' . $_SERVER['HTTP_HOST'];
          $notify = $form_state['values']['notify'];
          $command = "$drush --uri=$uri wordpress-migrate-import $destination --notify=$notify"
            . " >$log_file 2>&1 &";
          exec($command);
          $perform_import = FALSE;
        }
      }
    }
    if ($perform_import) {
      drupal_set_message(t('File %filename successfully uploaded and imported',
        array('%filename' => $filename)));
    }
  }
}

/**
 * Menu callback: Returns a page for configuring WordPress migrations.
 */
function wordpress_migrate_configure() {
  drupal_set_title(t('WordPress configuration'));
  return drupal_get_form('wordpress_migrate_configure_form');
}

/**
 * Form for configuring WordPress migrations.
 */
function wordpress_migrate_configure_form($form, &$form_state) {
  // Get writable streams
  $streams = file_get_stream_wrappers(STREAM_WRAPPERS_WRITE);

  $options = array();
  foreach ($streams as $scheme => $info) {
    $options[$scheme] = $info['name'];
  }

  $form['wordpress_migrate_scheme'] = array(
    '#type' => 'select',
    '#title' => t('Scheme for saving WordPress WXR dumps'),
    '#default_value' => variable_get('wordpress_migrate_scheme', 'private'),
    '#options' => $options,
    '#description' => t('It is recommended that you choose a scheme which does
      not allow public downloads. '),
  );

  $form['wordpress_migrate_drush'] = array(
    '#type' => 'textfield',
    '#title' => t('Location of drush command on server'),
    '#default_value' => variable_get('wordpress_migrate_drush', ''),
    '#description' => t('Larger blogs need to be imported by spawning a drush
      command. Please enter the full path of the drush command on the server
      to enable this functionality.'),
  );

  // Select destination node type(s)
  $node_types = node_type_get_types();
  $options = array();
  foreach ($node_types as $node_type => $info) {
    $options[$node_type] = $info->name;
  }

  if (isset($options['blog'])) {
    $default = 'blog';
  }
  else {
    $default = '';
  }

  $form['wordpress_migrate_post_type'] = array(
    '#type' => 'select',
    '#title' => t('Type of node to hold WordPress posts'),
    '#default_value' => variable_get('wordpress_migrate_post_type', $default),
    '#options' => $options,
    '#description' => t(''),
  );

  if (isset($options['page'])) {
    $default = 'page';
  }
  else {
    $default = '';
  }

  $form['wordpress_migrate_page_type'] = array(
    '#type' => 'select',
    '#title' => t('Type of node to hold WordPress pages'),
    '#default_value' => variable_get('wordpress_migrate_page_type', $default),
    '#options' => $options,
    '#description' => t(''),
  );

  // TODO: Select default text format for bodies etc.

  // TODO: Select user to own blog

  // Select vocabularies for categories and tags
  // TODO: Get only those attached to destination content types
  $vocabs = taxonomy_vocabulary_get_names();
  $options = array('' => t('Do not import'));
  foreach ($vocabs as $machine_name => $vocab) {
    $options[$machine_name] = $vocab->name;
  }

  $form['wordpress_migrate_tag_vocabulary'] = array(
    '#type' => 'select',
    '#title' => t('Vocabulary for WordPress tags'),
    '#default_value' => variable_get('wordpress_migrate_tag_vocabulary', ''),
    '#options' => $options,
    '#description' => t('Choose the vocabulary to hold WordPress tags.'),
  );

  $form['wordpress_migrate_category_vocabulary'] = array(
    '#type' => 'select',
    '#title' => t('Vocabulary for WordPress categories'),
    '#default_value' => variable_get('wordpress_migrate_category_vocabulary', ''),
    '#options' => $options,
    '#description' => t('Choose the vocabulary to hold WordPress categories.'),
  );

  // TODO: Select mechanism for running import (Batch API, cron, spawned process...)

  $form['wordpress_migrate_notification_subject'] = array(
    '#type' => 'textfield',
    '#title' => t('Subject for email notifications'),
    '#default_value' => variable_get('wordpress_migrate_notification_subject',
      t('Wordpress import status')),
    '#description' => t(''),
  );

  $form['wordpress_migrate_notification_body'] = array(
    '#type' => 'textarea',
    '#title' => t('Body for email notifications of import success'),
    '#default_value' => variable_get('wordpress_migrate_notification_body',
      t('Your WordPress import is complete!')),
  );

  $form['wordpress_migrate_notification_failure_body'] = array(
    '#type' => 'textarea',
    '#title' => t('Body for email notifications of import failure'),
    '#default_value' => variable_get('wordpress_migrate_notification_failure_body',
      t('Your WordPress import failed.')),
  );

  // TODO: For most of the above, indicate whether they can be overridden per import

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration changes'),
  );

  return $form;
}

/**
 * Submit callback for the WordPress configure form.
 */
function wordpress_migrate_configure_form_submit($form, &$form_state) {
  variable_set('wordpress_migrate_scheme', $form_state['values']['wordpress_migrate_scheme']);

  // TODO: Verify file exists
  variable_set('wordpress_migrate_drush', $form_state['values']['wordpress_migrate_drush']);

  variable_set('wordpress_migrate_post_type', $form_state['values']['wordpress_migrate_post_type']);
  variable_set('wordpress_migrate_page_type', $form_state['values']['wordpress_migrate_page_type']);
  variable_set('wordpress_migrate_tag_vocabulary', $form_state['values']['wordpress_migrate_tag_vocabulary']);
  variable_set('wordpress_migrate_category_vocabulary', $form_state['values']['wordpress_migrate_category_vocabulary']);
  variable_set('wordpress_migrate_notification_subject', $form_state['values']['wordpress_migrate_notification_subject']);
  variable_set('wordpress_migrate_notification_body', $form_state['values']['wordpress_migrate_notification_body']);
  variable_set('wordpress_migrate_notification_failure_body', $form_state['values']['wordpress_migrate_notification_failure_body']);
  drupal_set_message(t('WordPress configuration changes saved.'));
  $form_state['redirect'] = 'admin/content/wordpress';
}
