<?php

/**
 * @file
 *
 * Support for migrating comments from a WordPress blog into Drupal.
 */

/**
 * Implementation of MigrateSource, to handle migrating comments from WordPress XML dumps.
 */
class WordPressCommentSource extends WordPressSource {
  /**
   * List of available source fields.
   *
   * @var array
   */
  protected $fields = array(
    'post_id' => 'Unique ID of the item the comment is attached to',
    'comment_id' => 'Unique ID of the comment',
    'comment_author' => 'Name of comment author',
    'comment_author_email' => 'Email address of comment author',
    'comment_author_url' => 'URL of comment author',
    'comment_author_IP' => 'IP address from which comment was posted',
    'comment_date' => 'Date of comment (what timezone?)',
    'comment_date_gmt' => 'Date of comment (GMT)',
    'comment_content' => 'Body of comment',
    'comment_approved' => '1/0/spam - spam comments will not be imported',
    'comment_type' => '?',
    'comment_parent' => 'comment_id (?) of parent comment',
    'comment_user_id' => 'WordPress user ID of commenter (?)',
  );

  /**
   * Simple initialization.
   *
   */
  public function __construct($filename) {
    parent::__construct($filename);
    $this->xpath = '//channel/item/wp:comment';
  }

  /**
   * Parse the values out of the wp:comment element.
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

    // Find the containing item
    $post_id = $this->xml->xpath("//channel/item[wp:comment/wp:comment_id='" .
      $this->currentRow->comment_id . "']/wp:post_id");
    $this->currentRow->post_id = (string)$post_id[0];
    $this->currentKey = array($this->currentRow->comment_id);
    return TRUE;
  }
}

/**
 * Implementation of WordPressMigration, for comments
 */
class WordPressComment extends WordPressMigration {
  /**
   * Set it up
   */
  public function __construct(array $arguments = array()) {
    $arguments['post_type'] = 'comment_node_' . variable_get('wordpress_migrate_post_type', 'blog');
    parent::__construct($arguments);
    $this->dependencies = array($this->generateMachineName('WordPressBlogEntry'));
    $this->softDependencies = array($this->generateMachineName('WordPressPage'));

    // comment_id is the unique ID of items in WordPress
    $this->map = new MigrateSQLMap($this->machineName,
      array(
        'comment_id' => array(
          'type' => 'int',
          'unsigned' => TRUE,
          'not null' => TRUE,
          'description' => 'WordPress comment ID',
        )
      ),
      MigrateDestinationComment::getKeySchema()
    );

    $this->source = new WordPressCommentSource($this->wxrFile);
    $this->destination = new MigrateDestinationComment($arguments['post_type']);

    // The basic mappings
    $this->addFieldMapping('nid', 'post_id')
         ->sourceMigration(array($this->generateMachineName('WordPressBlogEntry'), $this->generateMachineName('WordPressPage')));
    $this->addFieldMapping('pid', 'comment_parent')
         ->sourceMigration($this->generateMachineName('WordPressComment'));
    $this->addFieldMapping('uid')
         ->description('Use email to match Drupal account; if no match, default to anonymous');
    $this->addFieldMapping('subject')
         ->description('No comment subjects in WP')
         ->issueGroup('DNM');
    $this->addFieldMapping('hostname', 'comment_author_IP');
    $this->addFieldMapping('created', 'comment_date');
    $this->addFieldMapping('changed', 'comment_date');
    $this->addFieldMapping(NULL, 'comment_date_gmt')
         ->description('Using comment_date')
         ->issueGroup('DNM');
    $this->addFieldMapping('status', 'comment_approved')
         ->description('Do not import those with values of "spam"');
    $this->addFieldMapping('thread')
         ->issueGroup('DNM');
    $this->addFieldMapping('name', 'comment_author');
    $this->addFieldMapping('mail', 'comment_author_email');
    $this->addFieldMapping('homepage', 'comment_author_url');
    $this->addFieldMapping('language');
    $this->addFieldMapping('path');
    $this->addFieldMapping(NULL, 'comment_id')
         ->description('Source primary key')
         ->issueGroup('DNM');
    $this->addFieldMapping(NULL, 'comment_user_id')
         ->description('Always 0?')
         ->issueGroup('DNM');
    $this->addFieldMapping(NULL, 'comment_type')
         ->description('Always empty?')
         ->issueGroup('DNM');
    $text_format = variable_get('wordpress_migrate_text_format', 'filtered_html');
    $arguments = array('format' => $text_format);
    $this->addFieldMapping('comment_body', 'comment_content')
         ->arguments($arguments);
  }

  public function prepareRow($row) {
    // Reject spam
    if ($row->comment_approved == 'spam') {
      return FALSE;
    }

    // Ignore pingbacks
    if ($row->comment_type == 'pingback') {
      return FALSE;
    }
    $row->comment_date = strtotime($row->comment_date);

    return TRUE;
  }

  /**
   * Prepare comment - called just before comment_save().
   *
   * @param stdClass $comment
   * @param stdClass $row
   */
  public function prepare(stdClass $comment, stdClass $row) {
    // Match creator email to Drupal account if possible; otherwise, use anonymous
    static $drupal_static_fast;
    if (!isset($drupal_static_fast)) {
      $drupal_static_fast['user_map'] = &drupal_static(__FUNCTION__);
    }
    $user_map = &$drupal_static_fast['user_map'];
    if (!isset($user_map[$row->comment_author_email])) {
      $user_map[$row->comment_author_email] = db_select('users', 'u')
                                              ->fields('u', array('uid'))
                                              ->condition('mail', $row->comment_author_email)
                                              ->execute()
                                              ->fetchField();
      if (!$user_map[$row->comment_author_email]) {
        $user_map[$row->comment_author_email] = 0;
      }
    }
    $comment->uid = $user_map[$row->comment_author_email];
  }
}
