<?php
// $Id$

/**
 * @file
 * Implementation of migration from WordPress into Drupal
 */
abstract class WordPressMigration extends DynamicMigration {
  /**
   * The filespec of the WXR file this migration is based on.
   *
   * @var string
   */
  protected $wxrFile;

  /**
   * The implemented WordPress migrations, in the order they should be run.
   */
  public static function migrationList() {
    return array(
      'WordPressCategory',
      'WordPressTag',
      'WordPressBlogEntry',
      'WordPressPage',
      'WordPressAttachment',
      'WordPressComment',
    );
  }

  /**
   * Constructor - general setup for WordPress migrations.
   *
   * @param array $arguments
   *  'filename' => WXR file managed by this migration
   */
  public function __construct(array $arguments) {
    if (!isset($arguments['filename'])) {
      throw new Exception(t('Filename is a required parameter for WordPress migrations'));
    }

    // Must be set before calling the parent constructor, which will call
    // generateMachineName() below.
    $this->wxrFile = $arguments['filename'];
    parent::__construct($arguments);
  }

  /**
   * Construct the machine name from the blog title
   */
  protected function generateMachineName($class_name = NULL) {
    $title = db_select('wordpress_migrate', 'wm')
             ->fields('wm', array('title'))
             ->condition('filename', $this->wxrFile)
             ->execute()
             ->fetchField();
    if (!$class_name) {
      $class_name = get_class($this);
    }

    // The machine name is the cleansed blog title, followed by the portion
    // of the class name after WordPress. For example, for
    // category migration (class WordPressCategory) of "Mike's Blog",
    // the generated machine name will be MikesBlogCategory.
    return self::machineFromTitle($class_name, $title);
  }

  protected static function machineFromTitle($class_name, $title) {
    $machine_name = $title . substr($class_name, strlen('WordPress'),
      strlen($class_name) - strlen('WordPress'));
    return $machine_name;
  }
}
