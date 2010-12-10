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
  $form['overview'] = array(
    '#prefix' => '<div>',
    '#markup' => "TBD: Describe the migration process. Make note of size restrictions
      for selecting local file (query PHP max values for guidance).",
    '#suffix' => '</div>',
  );

  $form['#attributes'] = array('enctype' => 'multipart/form-data');
  $form['wxr_file'] = array(
    '#type' => 'file',
    '#title' => t('WordPress exported (WXR) file to import into Drupal'),
  );

  // TODO: Select destination node type(s)

  // TODO: Select user to own blog

  // TODO: Select vocabulary for categories

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
  // TODO: Use configured file scheme, defaulting to private://
  $tmpfile = $_FILES['files']['tmp_name']['wxr_file'];
  $destination = '../private/wordpress/' . $_FILES['files']['name']['wxr_file'];
  $moved = move_uploaded_file($tmpfile, $destination);
  if ($moved) {
    // Get the full path where the file lives
    $destination = realpath($destination);

    // Extract the blog title, which will be used to construct migration machine names
    $xml = simplexml_load_file($destination);
    $title = (string)$xml->channel->title;
    // Keep only alphabetic characters
    // TODO: error if nothing's left
    $title = preg_replace('/[^A-Za-z]/', '', $title);

    // Write to the table.
    // TODO: What to do if this has previously been imported?
    db_merge('wordpress_migrate')
      ->key(array('filename' => $destination))
      ->fields(array('title' => $title))
      ->execute();

    // Instantiate each WP migration, passing the filename
    foreach (WordPressMigration::migrationList() as $class_name) {
      $migration = new $class_name(array('filename' => $destination));
      $migration->processImport();
    }
    drupal_set_message(t('File %filename successfully uploaded and imported',
      array('%filename' => $_FILES['files']['name']['wxr_file'])));
  }
  else {
    drupal_set_message(t('Failed to move file %filename', array('%filename' => $filename)));
  }
}
