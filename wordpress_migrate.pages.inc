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
  $post_type = variable_get('wordpress_migrate_post_type', '');
  if (!$post_type) {
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
  $directory = 'wordpress://';
  if (!file_prepare_directory($directory, FILE_CREATE_DIRECTORY | FILE_MODIFY_PERMISSIONS)) {
    form_set_error('wxr_file', t('Could not prepare directory %directory',
      array('%directory' => $directory)));
    return;
  }

  $tmpfile = $_FILES['files']['tmp_name']['wxr_file'];
  if ($tmpfile) {
    // Handle uploaded file
    $filename = $_FILES['files']['name']['wxr_file'];
    $destination = $directory . $filename;
    $saved = file_unmanaged_move($tmpfile, $destination, FILE_EXISTS_REPLACE);
    if (!$saved) {
      form_set_error('wxr_file', t('Failed to save file to %filename', array('%filename' => $destination)));
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
    try {
      $blog = wordpress_migrate_blog($destination);
    }
    catch (Exception $e) {
      form_set_error('wxr_file', $e->getMessage());
      return;
    }

    // Import each migration in order, until done or time is running out
    $spawned = FALSE;
    foreach ($blog->migrations() as $migration) {
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
          break;
        }
        drupal_set_message(t('The blog is too large to completely import immediately -
          the rest of the import will be run in the background and you will receive an email
          when it is complete'));
        $uri = 'http://' . $_SERVER['HTTP_HOST'];
        $log_file = '/tmp/' . $filename . '.import.log';
        $command = "$drush --uri=$uri wordpress-migrate-import $destination >$log_file 2>&1 &";
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
    'WordPressCategory' => array('data' => t('Categories')),
    'WordPressTag' => array('data' => t('Tags')),
    'WordPressBlogEntry' => array('data' => t('Posts')),
    'WordPressPage' => array('data' => t('Pages')),
    'WordPressAttachment' => array('data' => t('Attachments')),
    'WordPressComment' => array('data' => t('Comments')),
  );
  $rows = array();
  $blogs = call_user_func(array(wordpress_migrate_blog_class(), 'blogs'));
  foreach ($blogs as $blog) {
    $classes = array_flip($blog->migrationClasses());
    $row['blog_url'] = $blog->getBlogUrl();
    $row['status'] = t('Complete');
    foreach ($blog->migrations() as $migration) {
      $class = $classes[get_class($migration)];
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
    $blogs = array_keys($form_state['values']['blogs']);
    foreach ($blogs as $blog_url) {
      // TODO: Batch API
      // Rollback migrations for this domain
      $filename = db_select('wordpress_migrate', 'wp')
                   ->fields('wp', array('filename'))
                   ->condition('blog_url', $blog_url)
                   ->execute()
                   ->fetchField();
      $blog = wordpress_migrate_blog($filename);
      $migrations = array_reverse($blog->migrations());
      $success = TRUE;
      foreach ($migrations as $migration) {
        // Only rollback content for the rollback operation
        if ($form_state['values']['operation'] == 'clear') {
          // Remove map/message tables, and migrate_status table entry
          Migration::deregisterMigration($migration->getMachineName());
          $success = TRUE;
        }
        else {
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
          $result = $migration->processRollback();
          if ($result == MigrationBase::RESULT_INCOMPLETE) {
            $drush = variable_get('wordpress_migrate_drush', '');
            if (!$drush) {
              if (user_access(WORDPRESS_MIGRATE_ACCESS_CONFIGURE)) {
                $message = t('The blog was too large to delete through the browser - please
                      <a href="@config">configure drush</a> so the deletion process may be
                      run in the background.',
                array('@config' => url('admin/content/wordpress/configure')));
              }
              else {
                $message = t('The blog was too large to delete through the browser - please
                    contact a site administrator to properly configure the site for
                    background deletion.');
              }
              drupal_set_message($message);
              break;
            }
            drupal_set_message(t('The blog is too large to completely delete immediately -
                the rest of the deletion will be run in the background.'));
            $uri = 'http://' . $_SERVER['HTTP_HOST'];
            $log_file = '/tmp/' . basename($blog->getFilename()) . '.rollback.log';
            $destination = $blog->getFilename();
            $command = "$drush --uri=$uri wordpress-migrate-rollback $destination >$log_file 2>&1 &";
            exec($command);
            $success = FALSE;
            break;
          }
          elseif ($result == MigrationBase::RESULT_COMPLETED) {
            // Remove map/message tables, and migrate_status table entry
            Migration::deregisterMigration($migration->getMachineName());
          }
          else {
            drupal_set_message(t('Failed to complete rollback, status=!status',
              array('!status' => $result)));
            $success = FALSE;
            break;
          }
        }
      }

      if ($success) {
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
    }
    if ($success) {
      $count = count($form_state['values']['blogs']);
      watchdog('content', 'Deleted @count WordPress blogs.', array('@count' => $count));
      drupal_set_message(format_plural($count, 'Deleted 1 WordPress blog.',
        'Deleted @count WordPress blogs.'));
    }
  }
  $form_state['redirect'] = 'admin/content/wordpress/review';
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

  // Select default text format for bodies etc.
  $options = array();
  foreach (filter_formats() as $format_id => $format) {
    $options[$format_id] = $format->name;
  }
  $form['wordpress_migrate_text_format'] = array(
    '#type' => 'select',
    '#title' => t('Format for text fields'),
    '#default_value' => variable_get('wordpress_migrate_text_format', 'filtered_html'),
    '#options' => $options,
    '#description' => t(''),
  );

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
  // TODO: Verify drush command file exists
  variable_set('wordpress_migrate_drush', $form_state['values']['wordpress_migrate_drush']);

  variable_set('wordpress_migrate_post_type', $form_state['values']['wordpress_migrate_post_type']);
  variable_set('wordpress_migrate_text_format', $form_state['values']['wordpress_migrate_text_format']);
  variable_set('wordpress_migrate_page_type', $form_state['values']['wordpress_migrate_page_type']);
  variable_set('wordpress_migrate_tag_vocabulary', $form_state['values']['wordpress_migrate_tag_vocabulary']);
  variable_set('wordpress_migrate_category_vocabulary', $form_state['values']['wordpress_migrate_category_vocabulary']);
  variable_set('wordpress_migrate_notification_subject', $form_state['values']['wordpress_migrate_notification_subject']);
  variable_set('wordpress_migrate_notification_body', $form_state['values']['wordpress_migrate_notification_body']);
  variable_set('wordpress_migrate_notification_failure_body', $form_state['values']['wordpress_migrate_notification_failure_body']);
  drupal_set_message(t('WordPress configuration changes saved.'));
}
