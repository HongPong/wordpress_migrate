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
    return variable_get('file_private_path', '') . '/wordpress';
  }

  /**
   * Overrides getExternalUrl().
   *
   * Do not allow an external URL to WordPress files.
   */
  function getExternalUrl() {
    return FALSE;
  }
}
