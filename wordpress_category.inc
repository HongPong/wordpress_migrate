<?php
// $Id$

/**
 * @file
 *
 * Support for migrating categories from a WordPress blog into Drupal.
 */

/**
 * Implementation of MigrateSource, to handle migrating categories from WordPress XML dumps.
 */
class WordPressCategorySource extends WordPressSource {
  /**
   * List of available source fields.
   *
   * @var array
   */
  protected $fields = array(
    'category_nicename' => 'Unique "machine name" of the category',
    'category_parent' => 'Category parent (nicename?)',
    'cat_name' => 'User-friendly category name',
  );

  /**
   * Simple initialization.
   */
  public function __construct($filename) {
    parent::__construct($filename);
    $this->xpath = '//channel/wp:category';
  }

  /**
   * Parse the values out of the wp:category element.
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

    $this->currentKey = array($this->currentRow->category_nicename);
    return TRUE;
  }
}

/**
 * Implementation of WordPressMigration, for categories
 */
class WordPressCategory extends WordPressMigration {
  /**
   * Set it up
   */
  public function __construct(array $arguments = array()) {
    if (isset($arguments['vocabulary'])) {
      $vocabulary = $arguments['vocabulary'];
    }
    else {
      $vocabulary = 'tags';
    }
    // TODO: Validate presence of vocabulary
    parent::__construct($arguments);

    // The "nicename" is what we would call the machine name, the unique
    // identifier of a category.
    $this->map = new MigrateSQLMap($this->machineName,
      array(
        'category_nicename' => array(
          'type' => 'varchar',
          'length' => 255,
          'not null' => TRUE,
          'description' => 'WordPress category machine name',
        )
      ),
      MigrateDestinationTerm::getKeySchema()
    );

    // Construct the source and destination objects.
    $this->source = new WordPressCategorySource($this->wxrFile);
    $this->destination = new MigrateDestinationTerm($vocabulary);

    // The basic mappings
    $this->addFieldMapping('name', 'category_nicename');
    $this->addFieldMapping('description', 'cat_name');
    $this->addFieldMapping('parent_name', 'category_parent')
         ->description('Untested. What if parent isn\'t migrated before child?');

    // Unmapped destination fields
    $this->addFieldMapping('parent');
    $this->addFieldMapping('format');
    $this->addFieldMapping('weight');
    $this->addFieldMapping('path');
  }
}
