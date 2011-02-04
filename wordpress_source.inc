<?php
// $Id$

/**
 * @file
 * Common source support for migration from WordPress XML dumps.
 */

abstract class WordPressSource extends MigrateSource {
  /**
   * The WXR file being imported
   * @var string
   */
  protected $filename;

  /**
   * The parsed XML from the WXR file
   * @var SimpleXMLElement
   */
  protected $xml;
  public function getXml() {
    return $this->xml;
  }

  /**
   * Array of XML elements we're traversing for this source.
   *
   * @var array
   */
  protected $items = array();

  /**
   * Track where we are within the items array
   *
   * @var int
   */
  protected $itemIndex = 0;

  /**
   * Number of items to be processed
   * @var int
   */
  protected $itemCount = 0;

  /**
   * xpath for selecting the appropriate elements for this source.
   *
   * @var string
   */
  protected $xpath;

  /**
   * Number of eligible rows processed so far (used for itemlimit checking)
   *
   * @var int
   */
  protected $numProcessed = 0;

  /**
   * The domain name of this blog
   *
   * @var string
   */
  protected $baseBlogUrl;
  public function getBaseBlogUrl() {
    return $this->baseBlogUrl;
  }

  /**
   * Simple initialization.
   */
  public function __construct($filename) {
    parent::__construct();
    $this->filename = $filename;
    // Suppress errors during parsing, so we can pick them up after
    libxml_use_internal_errors(TRUE);
    $this->xml = simplexml_load_file($this->filename);
    if ($this->xml) {
      $base_blog_url = $this->xml->xpath('//channel/wp:base_blog_url');
      $this->baseBlogUrl = (string)$base_blog_url[0];
    }
    else {
      $message = '';
      foreach (libxml_get_errors() as $error) {
        $message .= $error->message . "\n";
      }
      throw new Exception($message);
    }
  }

  /**
   * Return a string representing the source.
   *
   * @return string
   */
  public function __toString() {
    return (string) $this->xpath;
  }

  /**
   * Returns a list of fields available to be mapped from the source.
   *
   * @return array
   *  Keys: machine names of the fields (to be passed to addFieldMapping)
   *  Values: Human-friendly descriptions of the fields.
   */
  public function fields() {
    return $this->fields;
  }

  /**
   * Return a count of all available source records.
   *
   * @param boolean $refresh
   *  Unused at this time.
   */
  public function count($refresh = FALSE) {
    $count = count($this->xml->xpath($this->xpath));
    return $count;
  }

  /**
   * Implementation of Iterator::rewind() - called before beginning a foreach loop.
   */
  public function rewind() {
    $this->numProcessed = 0;
    $this->items = $this->xml->xpath($this->xpath);
    $this->itemIndex = 0;
    $this->itemCount = count($this->items);

    // Load up the first row
    $this->next();
  }

  /**
   * Implementation of Iterator::next() - called at the bottom of the loop implicitly,
   * as well as explicitly from rewind().
   */
  public function next() {
    migrate_instrument_start('WordPressSource next');
    $migration = Migration::currentMigration();
    $this->currentRow = NULL;
    $this->currentKey = NULL;

    // If we've already hit the itemlimit, quit
    $itemlimit = $migration->getOption('itemlimit');
    if ($itemlimit && $this->numProcessed >= $itemlimit) {
      return;
    }

    // Loop over the elements to be processed. Note that itemIndex is persistent,
    // so subsequent next() calls pick up where we left off.
    while ($this->itemIndex < $this->itemCount) {
      $item = $this->items[$this->itemIndex++];

      $this->currentRow = NULL;

      // Each particular source type needs to parse things in its own way. If it
      // returns FALSE, skip this item.
      if (!$this->populateRow($item)) {
        continue;
      }

      // Previously migrated?
      $map_row = $migration->getMap()->getRowBySource($this->currentKey);
      if (!$map_row) {
        // Unmigrated row, take it
      }
      elseif ($map_row['needs_update'] == 1) {
        // We always want to take this row if needs_update = 1
      }
      else {
        // We want to skip this row if it's in the map table
        continue;
      }
      // Add map info to the row, if present
      if ($map_row) {
        foreach ($map_row as $field => $value) {
          $field = 'migrate_map_' . $field;
          $this->currentRow->$field = $value;
        }
      }
      // Allow the Migration to prepare this row. prepareRow() must return a
      // boolean FALSE to prevent the row from being processed.
      if (method_exists($migration, 'prepareRow')) {
        $success = $migration->prepareRow($this->currentRow);
      }
      else {
        $success = TRUE;
      }
      if ($success === FALSE) {
        $this->currentRow = NULL;
      }
      else {
        $this->numProcessed++;
        break;
      }
    }

    if (!is_object($this->currentRow)) {
      $this->currentRow = NULL;
    }
    migrate_instrument_stop('WordPressSource next');
  }
}
