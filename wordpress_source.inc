<?php

/**
 * @file
 * Common source support for migration from WordPress XML dumps.
 */

class WordPressXMLReader extends MigrateXMLReader {
  /**
   * Override of MigrateXMLReader::next().
   *
   * @return void
   */
  public function next() {
    parent::next();
    if (is_a($this->currentElement, 'SimpleXMLElement')) {
      $item = $this->currentElement;
      $currentRow = new stdClass;
      // Pull non-namespaced items
      foreach ($item as $name => $value) {
        $currentRow->$name = (string)$value;
      }
      $namespaces = $item->getNameSpaces(TRUE);
      foreach ($namespaces as $ns => $nsuri) {
        $item_ns = $item->children($namespaces[$ns]);
        foreach ($item_ns as $name => $value) {
          // Special-case content:encoded and excerpt:encoded, which otherwise
          // would both be saved as "encoded"
          if ($name == 'encoded') {
            $currentRow->$ns = (string)$value;
          }
          else {
            $currentRow->$name = (string)$value;
          }
        }
      }
      $this->currentElement = $currentRow;
    }
  }
}

/**
 * Extend MigrateSourceXML to move field data to the root of $row.
 */
class WordPressSourceXML extends MigrateSourceXML {
  public function getNextRow() {
    $row = parent::getNextRow();
    if (is_object($row)) {
      foreach ($row->xml as $field_name => $value) {
        $row->$field_name = $value;
      }
      unset($row->xml);
    }
    return $row;
  }
}
