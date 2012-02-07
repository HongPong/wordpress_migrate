<?php

/**
 * @file
 *
 * Support for migrating posts and pages from a WordPress blog into Drupal.
 */

/**
 * Implementation of MigrateSource, to handle migrating items from WordPress XML dumps.
 */
class WordPressItemSource extends WordPressSource {
  /**
   * The <wp:post_type> value we're looking for in this migration
   * (post/page/attachment).
   *
   * @var string
   */
  protected $postType;

  /**
   * List of available source fields.
   *
   * @var array
   */
  protected $fields = array(
    'title' => 'Item title',
    'link' => 'WordPress URL of the item',
    'pubDate' => 'Published date',
    'creator' => 'WordPress username of the item author',
    'guid' => 'Alternate URL of the item (?)',
    'description' => '?',
    'content' => 'Body of the item',
    'excerpt' => 'Teaser for the item',
    'post_id' => 'Unique ID of the item within the blog',
    'post_date' => 'Date posted (author\s timezone?)',
    'post_date_gmt' => 'Date posted (GMT)',
    'comment_status' => 'Whether comments may be posted to this item (open/closed)',
    'ping_status' => '?',
    'post_name' => 'Trailing component of link',
    'status' => 'Item status (publish/draft/inherit)',
    'post_parent' => 'Parent item ID (?)',
    'menu_order' => 'Equivalent to Drupal weight?',
    'post_type' => 'Item type (post/page/attachment)',
    'post_password' => '?',
    'is_sticky' => 'Equivalent to Drupal sticky flag',
    'category' => 'Categories (as nicename) assigned to this item',
    'tag' => 'Tags (as nicename) assigned to this item',
  );

  /**
   * Simple initialization.
   *
   */
  public function __construct($filename, $post_type) {
    parent::__construct($filename);
    $this->postType = $post_type;
    $this->xpath = '//channel/item';
  }

  /**
   * Return a count of all available source records.
   *
   * @param boolean $refresh
   *  Not currently in use.
   */
  public function count($refresh = FALSE) {
    $post_types = $this->xml->xpath("//channel/item[wp:post_type='$this->postType']");
    $count = count($post_types);
    return $count;
  }

  /**
   * Parse the values out of the item element.
   *
   * @param SimpleXMLElement $item
   * @return boolean
   */
  protected function populateRow($item) {
    // Pull non-namespaced items
    foreach ($item as $name => $value) {
      $this->currentRow->$name = (string)$value;
    }
    // Now check each namespace
    $namespaces = $item->getNameSpaces(TRUE);

    foreach ($namespaces as $ns => $nsuri) {
      $item_ns = $item->children($namespaces[$ns]);
      foreach ($item_ns as $name => $value) {
        // Special-case content:encoded and excerpt:encoded, which otherwise
        // would both be saved as "encoded"
        if ($name == 'encoded') {
          $this->currentRow->$ns = (string)$value;
        }
        else {
          $this->currentRow->$name = (string)$value;
        }
      }
    }
    // Make sure this is the desired post type
    if ($this->currentRow->post_type != $this->postType) {
      $this->currentRow = NULL;
      return FALSE;
    }

    // The category field is now the descriptive name, we want the nicename
    $this->currentRow->category = array();

    // The tag domain changed with WXR version 1.1
    $migration = Migration::currentMigration();
    $wxr_version = $this->xml->xpath('//channel/wp:wxr_version');
    $wxr_version = (float)reset($wxr_version);
    if ($wxr_version < 1.1) {
      $tag_domain = 'tag';
    }
    else {
      $tag_domain = 'post_tag';
    }

    foreach ($item->category as $cat) {
      $attributes = $cat->attributes();
      $domain = $attributes->domain;
      $nicename = $attributes->nicename;
      if ($nicename) {
        $nicename = (string)$nicename[0];
        if ($domain == 'category') {
          $this->currentRow->category[] = $nicename;
        }
        if ($domain == $tag_domain) {
          $this->currentRow->tag[] = $nicename;
        }
      }
    }

    $this->currentKey = array($this->currentRow->post_id);
    return TRUE;
  }
}

/**
 * Intermediate Migration class, implementing behavior common across different
 * types (post_type) of items.
 */
abstract class WordPressItemMigration extends WordPressMigration {
  /**
   * Indicates to the complete() method that the nid of this item needs to be
   * saved in linksToFix for later processing.
   *
   * @var boolean
   */
  protected $linksNeedFixing = FALSE;

  /**
   * List of nids which have links needing fixing.
   *
   * @var array
   */
  static protected $linksToFix = array();

  /**
   * Set it up
   */
  public function __construct(array $arguments = array()) {
    parent::__construct($arguments);

    $this->dependencies = array($this->generateMachineName('WordPressCategory'),
      $this->generateMachineName('WordPressTag'));

    // WordPress post type
    $post_type = $arguments['post_type'];
    // Drupal content type (bundle)
    $bundle = $arguments['bundle'];

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
      MigrateDestinationNode::getKeySchema()
    );

    // Construct the source objects.
    $this->source = new WordPressItemSource($this->wxrFile, $post_type);
    $this->destination = new MigrateDestinationNode($bundle);

    // Default mappings, applying to most or all migrations
    $this->addFieldMapping('title', 'title');
    $this->addFieldMapping('created', 'post_date')
         ->description('Empty dates handled in prepare()');
    $this->addFieldMapping('changed', 'post_date')
         ->description('Empty dates handled in prepare()');
    $this->addFieldMapping('uid', 'creator')
         ->description('Use matching username if any, otherwise current user');
    $text_format = variable_get('wordpress_migrate_text_format', 'filtered_html');
    $arguments = array('source_field' => 'excerpt', 'format' => $text_format);
    $this->addFieldMapping('body', 'content')
         ->arguments($arguments);
    $this->addFieldMapping(NULL, 'excerpt');
    $this->addFieldMapping('comment', 'comment_status')
         ->description('WP "open" mapped to Drupal COMMENT_NODE_OPEN');
    $this->addFieldMapping('status', 'status')
         ->description('Set Drupal status to 1 iff wp:status=publish');
    $this->addFieldMapping(NULL, 'post_parent')
         ->description('For attachments, indicates item attached to')
         ->issueGroup(t('Open issues'))
         ->issuePriority(MigrateFieldMapping::ISSUE_PRIORITY_MEDIUM);
    $this->addFieldMapping('sticky', 'is_sticky');
    if (module_exists('path')) {
      $this->addFieldMapping('path', 'link')
           ->description(t('If no ? in the link, strip the domain for the path'));
      if (module_exists('pathauto')) {
        $this->addFieldMapping('pathauto_perform_alias')
             ->defaultValue(0)
             ->description('Disable pathauto, we set the alias from the WP link');
      }
    }

    // Map the source fields to the configured vocabularies
    $vocabs = array(
      'tag' => variable_get('wordpress_migrate_tag_vocabulary', ''),
      'category' => variable_get('wordpress_migrate_category_vocabulary', ''),
    );

    // Look through this content type's fields for term references
    foreach ($this->destination->fields() as $machine_name => $description) {
      if (preg_match('/\(taxonomy_term_reference\)$/', $description)) {
        $field_info = field_info_field($machine_name);
        // Check this field against each of the configured vocabularies - if
        // a match is found, make the mapping
        foreach ($vocabs as $source_field => $vocab_name) {
          if ($vocab_name == $field_info['settings']['allowed_values'][0]['vocabulary']) {
            if ($source_field == 'tag') {
              $sourceMigration = $this->generateMachineName('WordPressTag');
            }
            else {
              $sourceMigration = $this->generateMachineName('WordPressCategory');
            }
            $this->addFieldMapping($machine_name, $source_field)
                 ->arguments(array('source_type' => 'tid'))
                 ->sourceMigration($sourceMigration);
            unset($vocabs[$source_field]);
          }
        }
      }
    }

    // If we didn't map one or both of the tag/category fields, indicate so with
    // a DNM mapping.
    foreach ($vocabs as $source_field => $vocab_name) {
      if ($vocab_name) {
        $message = t('!vocab_name vocabulary is not assigned to node type !bundle',
          array('!vocab_name' => $vocab_name, '!bundle' => $bundle));
      }
      else {
        $message = t('No vocabulary assigned for this field');
      }
      $this->addFieldMapping(NULL, $source_field)
           ->description($message)
           ->issueGroup(t('DNM'));
    }

    // Unmapped destination fields
    $this->addFieldMapping('is_new')
         ->issueGroup(t('DNM'));
    $this->addFieldMapping('revision')
         ->issueGroup(t('DNM'));
    $this->addFieldMapping('language')
         ->issueGroup(t('DNM'));
    $this->addFieldMapping('promote')
         ->issueGroup(t('DNM'));
    $this->addFieldMapping('revision_uid')
         ->issueGroup(t('DNM'));

    // Unmapped source fields
    $this->addFieldMapping(NULL, 'guid')
         ->description('same as link, plus isPermaLink attribute?')
         ->issueGroup(t('DNM'));
    $this->addFieldMapping(NULL, 'description')
         ->description('Always empty?')
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
    $this->addFieldMapping(NULL, 'ping_status')
         ->description('What does this mean?')
         ->issueGroup(t('Open issues'))
         ->issuePriority(MigrateFieldMapping::ISSUE_PRIORITY_MEDIUM);
    $this->addFieldMapping(NULL, 'post_name')
         ->description('Looks like last component of path')
         ->issueGroup(t('DNM'));
    $this->addFieldMapping(NULL, 'menu_order')
         ->issueGroup(t('DNM'));
    $this->addFieldMapping(NULL, 'post_type')
         ->issueGroup(t('DNM'));
    $this->addFieldMapping(NULL, 'post_password')
         ->description('???')
         ->issueGroup(t('DNM'));
  }

  /**
   * Data manipulations to be performed before the migrate module applies mappings.
   *
   * @param stdClass $row
   * @return string
   */
  public function prepareRow($row) {
    // Only publish those with wp:status == 'publish'
    if (isset($row->status) && $row->status == 'publish') {
      $row->status = NODE_PUBLISHED;
    }
    else {
      $row->status = NODE_NOT_PUBLISHED;
    }

    // If incoming date is zero (indicates unpublished content), use the current time
    if ($row->post_date == '0000-00-00 00:00:00') {
      $row->post_date = time();
    }

    // If the link has a query string, don't produce a path
    if (strpos($row->link, '?')) {
      unset($row->link);
    }
    else {
      // Otherwise, strip the domain portion of the URL
      $matches = array();
      if (preg_match('|http://[^/]+/(.*)|', $row->link, $matches)) {
        $row->link = $matches[1];
        // Strip the last slash off of the URL (the Path module can't handle this)
        $row->link = rtrim($row->link, '/');
      }
      else {
        unset($row->link);
      }
    }

    // Translate WordPress comment_status to Drupal values
    if ($row->comment_status == 'open') {
      $row->comment_status = COMMENT_NODE_OPEN;
    }
    else {
      $row->comment_status = COMMENT_NODE_CLOSED;
    }

    // Interpret the [caption] tags
    $row->content = preg_replace_callback('|(\[caption.*?\])(.*?)(\[/caption\])|i',
      array($this, 'replaceCaptions'), $row->content);

    // Rewrite embedded video references to media tags (TODO: YouTube only for now)
    if (module_exists('media_youtube')) {
      $row->content = preg_replace_callback('|<object [^>]*>.*?(<embed [^>]*>).*?</object>|i',
        array($this, 'replaceEmbeds'), $row->content);
    }

    // Rewrite (or remember to rewrite) links of the form
    // http://example.wordpress.com/?p=19 to local links of the form /node/35
    $row->content = $this->fixLocalLinks($row->content);

    return TRUE;
  }

  /**
   * Rewrite [caption] tags into HTML representing a caption.
   * [caption] itself ($matches[1]) will become an opening <div>,
   * the content within the tag ($matches[2]) will be passed through unchanged,
   * and the closing [/caption] ($matches[3]) will become a <p> containing the
   * caption followed by a closing </div>.
   *
   * @param array $matches
   */
  protected function replaceCaptions(array $matches) {
    $caption_open = $matches[1];
    $content = $matches[2];
    $caption_close = $matches[3];

    preg_match('|width="(.*?)"|i', $caption_open, $matches);
    $width = (int)$matches[1] + 10;
    $style = "width: {$width}px;";

    preg_match('|align="(.*?)"|i', $caption_open, $matches);
    $align = $matches[1];
    switch ($align) {
      case 'aligncenter':
        $style .= "display:block;margin:0 auto;";
        break;
      case 'alignleft':
        $style .= "float:left;";
        break;
      case 'alignright':
        $style .= "float:right;";
        break;
      default:
        break;
    }
    preg_match('|caption="(.*?)"|i', $caption_open, $matches);
    $caption = $matches[1];

    $result = '<div style="' . $style . '">';
    $result .= $content;
    $result .= "<p>$caption</p></div>";

    return $result;
  }

  /**
   * If we have a YouTube reference, replace it with media tags.
   *
   * @param array $matches
   */
  protected function replaceEmbeds(array $matches) {
    $embed_matches = array();
    // Default to the original <object> tag
    $result = $matches[0];

    // If an <embed> tag is present, parse it
    if ($matches[1]) {
      if (preg_match('|src=[\'"](.*?)[\'"]|i', $matches[1], $src_matches)) {
        $src = $src_matches[1];
      }
      else {
        return $result;
      }
      // We support YouTube only
      if (preg_match('|^https?://www.youtube.com/|', $src)) {
        $src = preg_replace('|^https?://www.youtube.com/|', 'youtube://', $src);
      }
      else {
        return $result;
      }
      if (preg_match('|width=[\'"](.*?)[\'"]|i', $matches[1], $width_matches)) {
        $width = $width_matches[1];
      }
      else {
        return $result;
      }
      if (preg_match('|height=[\'"](.*?)[\'"]|i', $matches[1], $height_matches)) {
        $height = $height_matches[1];
      }
      else {
        return $result;
      }
      // OK, at this point we have all the info we need - construct a file_managed table
      // entry (if one doesn't already exist)
      $fid = db_select('file_managed', 'f')
             ->fields('f', array('fid'))
             ->condition('uri', $src)
             ->execute()
             ->fetchField();
      if (!$fid) {
        global $user;
        $file = new stdClass();
        $file->uri = $src;
        $file->filename = basename($src);
        $file->filemime = 'video/youtube';
        $file->type = 'video';
        $file->uid = $user->uid;
        $file->status = 1;
        file_save($file);
        if (module_exists('media')) {
          file_save($file);
        }
        $fid = $file->fid;
      }
      // Build the media tag
      $video_info = array(
        'type' => 'media',
        'view_mode' => 'media_large',
        'fid' => $fid,
        'attributes' => array(
          'class' => 'media-image',
          'typeof' => 'foaf:Image',
          'height' => $height,
          'width' => $width,
          'style' => '',
        ),
      );
      $result = '[[' . drupal_json_encode($video_info) . ']]';
    }

    return $result;
  }

  /**
   * Replace any hrefs to links of the form http://example.wordpress.com/?=23
   * to local links to a node.
   *
   * @param string $body
   */
  protected function fixLocalLinks($body) {
    $this->linksNeedFixing = FALSE;
    $site_url = reset($this->source->getXml()->xpath('//channel/link'));
    $pattern = '|href="' . $site_url . '/\?p=([0-9]+)"|i';
    $body = preg_replace_callback($pattern, array($this, 'replaceLinks'), $body);
    return $body;
  }

  /**
   * If we have a local link of the form ?p=34, translate the WordPress ID into
   * a Drupal nid, and rewrite the link.
   *
   * @param array $matches
   */
  protected function replaceLinks(array $matches) {
    // Default to the existing string
    $return = $matches[0];

    $wordpress_id = (int)$matches[1];

    // Check the blog entry and page maps to see if we can map this to a nid
    static $maps = array();
    if (empty($maps)) {
      $machines = array($this->generateMachineName('WordPressBlogEntry'),
        $this->generateMachineName('WordPressPage'));
      foreach ($machines as $machine) {
        $maps[] = MigrationBase::getInstance($machine)->getMap();
      }
    }

    foreach ($maps as $map) {
      $destination_id = $map->lookupDestinationID(array($wordpress_id), $this);
      if (!empty($destination_id)) {
        // Got a hit! Stop looking...
        $destination_id = reset($destination_id);
        break;
      }
    }

    // Remember if we didn't get a hit, complete() will set up for later review
    if (empty($destination_id)) {
      $this->linksNeedFixing = TRUE;
    }
    else {
      $return = 'href="/node/' . $destination_id . '"';
    }
    return $return;
  }

  /**
   * Prepare node - called just before node_save().
   *
   * @param stdClass $node
   * @param stdClass $row
   */
  public function prepare(stdClass $node, stdClass $row) {
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
    $node->uid = $user_map[$row->creator];
  }

  /**
   * Complete node - called just after node_save().
   *
   * @param stdClass $node
   * @param stdClass $row
   */
  public function complete(stdClass $node, stdClass $row) {
    // Remember the nid of any node where we weren't able to resolve ?p=23
    // links yet - by the time the page migration's postImport() is called, we
    // should have resolved all references.
    if ($this->linksNeedFixing) {
      self::$linksToFix[] = $node->nid;
    }
  }
}

/**
 * Implementation of WordPressMigration, for blog entries
 */
class WordPressBlogEntry extends WordPressItemMigration {
  public function __construct(array $arguments = array()) {
    $arguments['post_type'] = 'post';
    $arguments['bundle'] = variable_get('wordpress_migrate_post_type', '');
    parent::__construct($arguments);
  }
}

/**
 * Implementation of WordPressMigration, for pages
 */
class WordPressPage extends WordPressItemMigration {
  public function __construct(array $arguments = array()) {
    $arguments['post_type'] = 'page';
    $arguments['bundle'] = variable_get('wordpress_migrate_page_type', '');
    parent::__construct($arguments);
    $this->dependencies += array($this->generateMachineName('WordPressBlogEntry'));
  }

  /**
   * Called after completion of the page migration. We review any nodes with links
   * that couldn't be resolved at migration time (presumably because they refer to
   * nodes not yet migrated) and see if we can resolve them now.
   */
  public function postImport() {
    foreach (self::$linksToFix as $nid) {
      $node = node_load($nid);
      $node->body[LANGUAGE_NONE][0]['value'] = $this->fixLocalLinks($node->body[LANGUAGE_NONE][0]['value']);
      node_save($node);
    }
  }
}
