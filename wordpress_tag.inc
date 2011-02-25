<?php

/**
 * @file
 *
 * Support for migrating tags from a WordPress blog into Drupal.
 */

/**
 * Implementation of MigrateSource, to handle migrating tags from WordPress XML dumps.
 */
class WordPressTagSource extends WordPressSource {
  /**
   * List of available source fields.
   *
   * @var array
   */
  protected $fields = array(
    'tag_slug' => 'Unique "machine name" of the tag',
    'tag_name' => 'User-friendly tag name',
    'tag_description' => 'Description of tag',
  );

  /**
   * Simple initialization.
   */
  public function __construct($filename) {
    parent::__construct($filename);
    $this->xpath = '//channel/wp:tag';
  }

  /**
   * Parse the values out of the wp:tag element.
   *
   * @param SimpleXMLElement $item
   * @return boolean
   */
  protected function populateRow($item) {
    // All the stuff we want is in the wp namespace
    $namespaces = $item->getNameSpaces(TRUE);
    $item_ns = $item->children($namespaces['wp']);
    foreach ($item_ns as $name => $value) {
      $this->currentRow->$name = (string)$value;
    }

    $this->currentKey = array($this->currentRow->tag_slug);
    return TRUE;
  }
}

/**
 * Implementation of WordPressMigration, for tags
 */
class WordPressTag extends WordPressMigration {
  /**
   * Set it up
   */
  public function __construct(array $arguments = array()) {
    parent::__construct($arguments);

    // The slug is what we would call the machine name, the unique
    // identifier of a tag.
    $this->map = new MigrateSQLMap($this->machineName,
      array(
        'tag_slug' => array(
          'type' => 'varchar',
          'length' => 255,
          'not null' => TRUE,
          'description' => 'WordPress tag machine name',
        )
      ),
      MigrateDestinationTerm::getKeySchema()
    );

    // Construct the source and destination objects.
    $this->source = new WordPressTagSource($this->wxrFile);
    $this->destination = new MigrateDestinationTerm(
      variable_get('wordpress_migrate_tag_vocabulary', ''));

    // The basic mappings
    $this->addFieldMapping('name', 'tag_name');
    $this->addFieldMapping('description', 'tag_description');
    $this->addFieldMapping('parent_name')
         ->issueGroup('DNM');

    // Unmapped destination fields
    $this->addFieldMapping('parent');
    $this->addFieldMapping('format');
    $this->addFieldMapping('weight');
    $this->addFieldMapping('path');
  }
}
