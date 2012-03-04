<?php

/**
 * Override WordPressItemSource, to add additional fields for attachments.
 */
class WordPressAttachmentSource extends WordPressItemSource {
  /**
   * List of additional source fields for attachments.
   *
   * @var array
   */
  protected $attachmentFields = array(
    'attachment_url' => 'The URL of the attached file',
    '_wp_attached_file' => 'Local (to WordPress) filespec',
    '_wp_attachment_metadata' => 'Serialized metadata (image size etc.)',
  );

  /**
   * Simple initialization.
   *
   */
  public function __construct($filename) {
    parent::__construct($filename, 'attachment');
    $this->fields += $this->attachmentFields;
  }
}

/**
 * Implementation of WordPressMigration, for attachments
 */
class WordPressAttachment extends WordPressMigration {
  /**
   * Set it up
   */
  public function __construct(array $arguments = array()) {
    parent::__construct($arguments);

    $this->dependencies = array($this->generateMachineName('WordPressBlogEntry'),
      $this->generateMachineName('WordPressPage'));

    $dest_options = array('copy_file' => TRUE);
    // Make use of the media module if present
    if (module_exists('media') && module_exists('migrate_extras')) {
      $dest_options['source_migrations'] = $this->dependencies;
      $this->destination = new MigrateDestinationMedia('image', $dest_options);
      $keySchema = MigrateDestinationMedia::getKeySchema();
      $this->addFieldMapping('media_title', 'title');
      $this->addFieldMapping('media_description', 'content');
      $this->addFieldMapping('file')
           ->issueGroup(t('DNM'));
    }
    else {
      $this->destination = new MigrateDestinationFile($dest_options);
      $keySchema = MigrateDestinationFile::getKeySchema();
      $this->addFieldMapping(NULL, 'title')
           ->issueGroup(t('DNM'));
      $this->addFieldMapping(NULL, 'content')
           ->issueGroup(t('DNM'));
    }

    // post_id is the unique ID of items in WordPress
    $this->map = new MigrateSQLMap($this->machineName,
      array(
        'post_id' => array(
          'type' => 'int',
          'not null' => TRUE,
          'unsigned' => TRUE,
          'description' => 'WordPress post ID',
        )
      ),
      $keySchema
    );

    // Construct the source object.
    $this->source = new WordPressAttachmentSource($this->wxrFile);

    $this->addFieldMapping('fid')
         ->issueGroup(t('DNM'));
    $this->addFieldMapping('uid', 'creator')
         ->description('Use matching username if any, otherwise current user');
    $this->addFieldMapping('filename')
         ->description(t('Defaults to basename(uri)'));
    $this->addFieldMapping('uri', 'attachment_url');
    $this->addFieldMapping('filemime')
         ->description(t('Determined by default'));
    $this->addFieldMapping('status')
         ->defaultValue(FILE_STATUS_PERMANENT);
    $this->addFieldMapping('timestamp', 'post_date');
    $this->addFieldMapping(NULL, 'post_parent')
         ->description('TODO: find node we\'re attached to, see if it has filefield')
         ->issueGroup(t('Open issues'))
         ->issuePriority(MigrateFieldMapping::ISSUE_PRIORITY_MEDIUM);

    // Unmapped destination fields
    $this->addFieldMapping('path')
         ->issueGroup(t('DNM'));

    // Unmapped source fields
    $this->addFieldMapping(NULL, 'link')
         ->issueGroup(t('DNM'));
    $this->addFieldMapping(NULL, 'guid')
         ->issueGroup(t('DNM'));
    $this->addFieldMapping(NULL, 'description')
         ->issueGroup(t('DNM'));
    $this->addFieldMapping(NULL, 'excerpt')
         ->issueGroup(t('DNM'));
    $this->addFieldMapping(NULL, 'post_id')
         ->description(t('Primary key of source'))
         ->issueGroup(t('DNM'));
    $this->addFieldMapping(NULL, 'pubDate')
         ->description('Use post_date')
         ->issueGroup(t('DNM'));
    $this->addFieldMapping(NULL, 'post_date_gmt')
         ->description('Use post_date')
         ->issueGroup(t('DNM'));
    $this->addFieldMapping(NULL, 'comment_status')
         ->issueGroup(t('DNM'));
    $this->addFieldMapping(NULL, 'ping_status')
         ->issueGroup(t('DNM'));
    $this->addFieldMapping(NULL, 'post_name')
         ->issueGroup(t('DNM'));
    $this->addFieldMapping(NULL, 'status')
         ->issueGroup(t('DNM'));
    $this->addFieldMapping(NULL, 'menu_order')
         ->issueGroup(t('DNM'));
    $this->addFieldMapping(NULL, 'post_type')
         ->issueGroup(t('DNM'));
    $this->addFieldMapping(NULL, 'post_password')
         ->issueGroup(t('DNM'));
    $this->addFieldMapping(NULL, 'is_sticky')
         ->issueGroup(t('DNM'));
    $this->addFieldMapping(NULL, 'category')
         ->issueGroup(t('DNM'));
    $this->addFieldMapping(NULL, '_wp_attached_file')
         ->issueGroup(t('DNM'));
    $this->addFieldMapping(NULL, '_wp_attachment_metadata')
         ->issueGroup(t('DNM'));
  }

  /**
   * Data manipulations to be performed before migrate module applies mappings.
   *
   * @param stdClass $row
   * @return string
   */
  public function prepareRow($row) {
    // Skip any of the wrong post type
    if ($row->post_type != 'attachment') {
      return FALSE;
    }

    // If incoming date is zero (indicates unpublished content), use the current time
    if ($row->post_date == '0000-00-00 00:00:00') {
      $row->post_date = time();
    }
    return TRUE;
  }

  /**
   * Prepare node - called just before node_save().
   *
   * @param stdClass $node
   * @param stdClass $row
   */
  public function prepare(stdClass $file, stdClass $row) {
    // Match creator username to Drupal username if possible; otherwise, use
    // the user that initiated the import
    static $drupal_static_fast;
    if (!isset($drupal_static_fast)) {
      $drupal_static_fast['user_map'] = &drupal_static(__FUNCTION__);
      $drupal_static_fast['default_user'] = &drupal_static(__FUNCTION__ . 'DefaultUser');
    }
    $user_map = &$drupal_static_fast['user_map'];
    if (!isset($user_map[$row->creator])) {
      $user_map[$row->creator] = db_select('users', 'u')
                                 ->fields('u', array('uid'))
                                 ->condition('name', $row->creator)
                                 ->execute()
                                 ->fetchField();
      if (!$user_map[$row->creator]) {
        $default_user = &$drupal_static_fast['default_user'];
        if (!isset($default_user)) {
          $default_user = db_select('wordpress_migrate', 'wpm')
                          ->fields('wpm', array('uid'))
                          ->condition('filename', $this->wxrFile)
                          ->execute()
                          ->fetchField();
        }
        $user_map[$row->creator] = $default_user;
      }
    }
    $file->uid = $user_map[$row->creator];
  }

  /**
   * Called after file object is saved - maintain a mapping from the URL on the
   * original WordPress blog to the URI in Drupal.
   *
   * @param stdClass $file
   * @param stdClass $row
   */
  public function complete(stdClass $file, stdClass $row) {
    db_merge('wordpress_migrate_attachment')
      ->key(array('filename' => $this->wxrFile, 'original_url' => $row->attachment_url))
      ->fields(array('new_uri' => parse_url(file_create_url($file->uri), PHP_URL_PATH)))
      ->execute();

    // If media_gallery is enabled, add this image to the user's gallery.
    // Lazy-create the gallery node if it doesn't already exist
    // TODO: Needs generalization, takes for granted blog module
    // TODO: Cache fids to add, do them all at once
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
        $gallery_node = node_load($gallery_nid);
      }
      else {
        $gallery_node = new stdClass;
        $gallery_node->type = 'media_gallery';
        $gallery_node->title = $blog_title;
        $gallery_node->uid = $user->uid;
        $gallery_node->language = LANGUAGE_NONE;
      }
      $gallery_node->media_gallery_media[LANGUAGE_NONE][] = array('fid' => $file->fid);
      node_save($gallery_node);
    }
  }

  /**
   * Called after all attachments are imported - fix up references to the imported
   * attachments in node bodies.
   */
  public function postImport() {
    parent::postImport();
    migrate_instrument_start('WordPressAttachment postImport');
    $source_migrations = array($this->generateMachineName('WordPressBlogEntry'),
      $this->generateMachineName('WordPressPage'));
    foreach ($source_migrations as $source_migration) {
      $migration = Migration::getInstance($source_migration);
      $map = $migration->getMap();
      foreach ($map as $map_row) {
        $nid = $map_row->destid1;
        if ($nid) {
          $node = node_load($nid);
          $body = $node->body[LANGUAGE_NONE][0]['value'];
          $result = db_select('wordpress_migrate_attachment', 'a')
                    ->fields('a', array('original_url', 'new_uri'))
                    ->condition('filename', $this->wxrFile)
                    ->execute();
          foreach ($result as $row) {
            $body = str_replace($row->original_url, $row->new_uri, $body);
          }
          $node->body[LANGUAGE_NONE][0]['value'] = $body;
          // Maintain the original update datestamp
          $changed = $node->changed;
          node_save($node);
          db_update('node')
            ->fields(array('changed' => $changed))
            ->condition('nid', $node->nid)
            ->execute();
        }
      }
    }
    migrate_instrument_stop('WordPressAttachment postImport');
  }

  /**
   * Remove all attachment records
   */
  public function postRollback() {
    db_delete('wordpress_migrate_attachment')
    ->condition('filename', $this->wxrFile)
    ->execute();
  }
}
