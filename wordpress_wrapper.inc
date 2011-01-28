<?php

/**
 * Wordpress Migrate (wordpress://) stream wrapper class.
 *
 * Provides support for storing uploaded WXR files where they won't be seen by
 * anyone but the import code.
 *
 * Extends DrupalLocalStreamWrapper.
 */
class WordpressMigrateStreamWrapper extends DrupalLocalStreamWrapper {
  /**
   * Implements abstract public function getDirectoryPath()
   */
  public function getDirectoryPath() {
    return variable_get('file_public_path', conf_path() . '/files') . '/wordpress';
  }

  /**
   * Overrides getExternalUrl().
   *
   * Return the HTML URI of a private file.
   */
  function getExternalUrl() {
    $path = str_replace('\\', '/', $this->getTarget());
    return url('system/files/' . $path, array('absolute' => TRUE));
  }
}
