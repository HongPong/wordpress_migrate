<?php
// $Id$

/**
 * @file
 * Drush support for the wordpress_migrate module
 */

/**
 * Implementation of hook_drush_help().
 */
function wordpress_migrate_drush_help($section) {
  switch ($section) {
    case 'drush:wordpress-migrate-import':
      return dt('Import a WordPress blog');
  }
}

/**
 * Implementation of hook_drush_command().
 */
function wordpress_migrate_drush_command() {
  static $commands = FALSE;

  $items['wordpress-migrate-import'] = array(
    'description' => 'Import a WordPress blog',
    'arguments' => array(
      'filename' => 'Filename of blog export to import',
    ),
    'options' => array('--notify' => 'Email address to notify upon completion'),
    'examples' => array(
      'wordpress-migrate-import public://wordpress/blog.xml' => 'Import blog data',
    ),
    'drupal dependencies' => array('migrate'),
  );
  return $items;
}

function drush_wordpress_migrate_import($filename) {
  $row = db_select('wordpress_migrate', 'wm')
         ->fields('wm', array('title', 'uid'))
         ->condition('filename', $filename)
         ->execute()
         ->fetchObject();
  if ($row) {
    global $user;
    $user = user_load($row->uid);
    $migration_classes = WordPressMigration::migrationList();
    foreach ($migration_classes as $class_name) {
      $machine_name = _wordpress_machine_from_title($class_name, $row->title);
      $migration = MigrationBase::getInstance($machine_name);
      $result = $migration->processImport();
      // TODO: Validate result
    }
    // Notify user
    $notify = drush_get_option('notify');
    if ($notify) {
      $params['account'] = $user;
      drupal_mail('wordpress_migrate', 'import_complete', $notify,
        user_preferred_language($user), $params);
    }
  }
  else {
    // TODO: setup new migration from the URL
    drush_log(dt('No WordPress migration defined for !filename',
      array('!filename' => $filename)), 'error');
  }
}
