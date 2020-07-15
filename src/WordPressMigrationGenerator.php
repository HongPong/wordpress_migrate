<?php

namespace Drupal\wordpress_migrate;

use Drupal\migrate_plus\Entity\Migration;
use Drupal\migrate_plus\Entity\MigrationGroup;

/**
 * Functionality to construct WordPress migrations from broad configuration
 * settings.
 */
class WordPressMigrationGenerator {

  /**
   * Configuration to guide our migration creation process.
   *
   * @var array
   */
  protected $configuration = [];

  /**
   * Process plugin configuration for uid references.
   *
   * @var array
   */
  protected $uidMapping = [];

  /**
   * ID of the configuration entity for author migration.
   *
   * @var string
   */
  protected $authorID = '';

  /**
   * ID of the configuration entity for tag migration.
   *
   * @var string
   */
  protected $tagsID = '';

  /**
   * ID of the configuration entity for category migration.
   *
   * @var string
   */
  protected $categoriesID = '';

  /**
   * ID of the configuration entity for attachment migration.
   *
   * @var string
   */
  protected $attachmentID = '';

  /**
   * Constructs a WordPress migration generator, using provided configuration.
   *
   * @param array $configuration
   *   An associative array:
   *   - file_uri: Drupal stream wrapper of the source WordPress XML file.
   *   - group_id: ID of the MigrationGroup holding the generated migrations.
   *   - prefix: String to prefix to the IDs of generated migrations.
   *   - default_author: If present, username to author all imported content. If
   *     absent or empty, users will be imported from WordPress.
   *   - tag_vocabulary: Machine name of vocabulary to hold tags.
   *   - category_vocabulary: Machine name of vocabulary to hold categories.
   *   - [post|page]: Associative array of type-specific configuration:
   *     - type: Machine name of Drupal node bundle to hold content.
   *     - text_format: Machine name of text format for body field.
   * @todo: Validate inputs (e.g., make sure post type exists).
   * @link https://www.drupal.org/node/2742283
   */
  public function __construct(array $configuration) {
    $this->configuration = $configuration;
  }

  /**
   * Creates a set of WordPress import migrations based on configuration
   * settings.
   *
   * @throws \Drupal\Core\Entity\EntityStorageException
   * @throws \Exception
   */
  public function createMigrations() {
    // Create the migration group.
    $group_configuration = [
      'id' => $this->configuration['group_id'],
      // @todo: Add Wordpress site title in here.
      // @link https://www.drupal.org/node/2742287
      'label' => 'Imports from WordPress site',
      'source_type' => 'WordPress',
      'shared_configuration' => [
        'source' => [
          // @todo: Dynamically populate from the source XML.
          // @link https://www.drupal.org/node/2742287
          'namespaces' => [
            'wp' => 'http://wordpress.org/export/1.2/',
            'excerpt' => 'http://wordpress.org/export/1.2/excerpt/',
            'content' => 'http://purl.org/rss/1.0/modules/content/',
            'wfw' => 'http://wellformedweb.org/CommentAPI/',
            'dc' => 'http://purl.org/dc/elements/1.1/',
          ],
          'urls' => [
            $this->configuration['file_uri'],
          ],
        ],
      ],
    ];
    MigrationGroup::create($group_configuration)->save();

    // Determine the uid mappings, creating an author migration if needed.
    if ($this->configuration['default_author']) {
      $account = user_load_by_name($this->configuration['default_author']);
      if ($account) {
        $this->uidMapping = [
          'plugin' => 'default_value',
          'default_value' => $account->id(),
        ];
      }
      else {
        throw new \Exception('Username @name does not exist.',
          ['@name' => $this->configuration['default_author']]);
      }
    }
    else {
      $this->authorID = $this->configuration['prefix'] . 'wordpress_authors';
      $migration      = static::createEntityFromPlugin('wordpress_authors', $this->authorID);
      $migration->set('migration_group', $this->configuration['group_id']);
      $migration->save();
      $this->uidMapping = [
        'plugin' => 'migration_lookup',
        'migration' => $this->authorID,
        'source' => 'creator',
      ];
    }

    // Set up the attachment migration.
    $this->attachmentID = $this->configuration['prefix'] . 'wordpress_attachments';
    $migration = static::createEntityFromPlugin('wordpress_attachments', $this->attachmentID);
    $migration->set('migration_group', $this->configuration['group_id']);
    $process = $migration->get('process');
    $process['uid'] = $this->uidMapping;
    $migration->set('process', $process);
    $migration->save();

    // Setup vocabulary migrations if requested.
    if ($this->configuration['tag_vocabulary']) {
      $this->tagsID = $this->configuration['prefix'] . 'wordpress_tags';
      $migration    = static::createEntityFromPlugin('wordpress_tags', $this->tagsID);
      $migration->set('migration_group', $this->configuration['group_id']);
      $process        = $migration->get('process');
      $process['vid'] = [
        'plugin' => 'default_value',
        'default_value' => $this->configuration['tag_vocabulary'],
      ];
      $migration->set('process', $process);
      $migration->save();
    }
    if ($this->configuration['category_vocabulary']) {
      $this->categoriesID = $this->configuration['prefix'] . 'wordpress_categories';
      $migration          = static::createEntityFromPlugin('wordpress_categories', $this->categoriesID);
      $migration->set('migration_group', $this->configuration['group_id']);
      $process        = $migration->get('process');
      $process['vid'] = [
        'plugin' => 'default_value',
        'default_value' => $this->configuration['category_vocabulary'],
      ];
      $migration->set('process', $process);
      $migration->save();
    }

    // Setup the content migrations.
    foreach (['post', 'page'] as $wordpress_type) {
      if (!empty($this->configuration[$wordpress_type]['type'])) {
        $this->createContentMigration($wordpress_type);
      }
    }
  }

  /**
   * Setup the migration for a given WordPress content type.
   *
   * @param string $wordpress_type
   *   WordPress content type - 'post' or 'page'.
   *
   * @throws \Drupal\Core\Entity\EntityStorageException
   * @throws \Drupal\Component\Plugin\Exception\PluginException
   */
  protected function createContentMigration($wordpress_type) {
    $dependencies = [];
    $content_id   = $this->configuration['prefix'] . 'wordpress_content_' . $wordpress_type;
    $migration    = static::createEntityFromPlugin('wordpress_content', $content_id);
    $migration->set('migration_group', $this->configuration['group_id']);
    $source                   = $migration->get('source');
    $source['item_selector'] .= '[wp:post_type="' . $wordpress_type . '"]';
    $migration->set('source', $source);
    $process                = $migration->get('process');
    $process['uid']         = $this->uidMapping;
    $process['body/format'] = [
      'plugin' => 'default_value',
      'default_value' => $this->configuration[$wordpress_type]['text_format'],
    ];
    $process['type']        = [
      'plugin' => 'default_value',
      'default_value' => $this->configuration[$wordpress_type]['type'],
    ];
    if ($this->configuration['tag_vocabulary']) {
      $term_field = $this->termField($this->configuration[$wordpress_type]['type'], $this->configuration['tag_vocabulary']);
      if ($term_field) {
        $process[$term_field] = [
          'plugin' => 'migration_lookup',
          'migration' => $this->tagsID,
          'source' => 'post_tag',
        ];
        $dependencies[]       = $this->tagsID;
      }
    }
    if ($this->configuration['category_vocabulary']) {
      $term_field = $this->termField($this->configuration[$wordpress_type]['type'], $this->configuration['category_vocabulary']);
      if ($term_field) {
        $process[$term_field] = [
          'plugin' => 'migration_lookup',
          'migration' => $this->categoriesID,
          'source' => 'category',
        ];
        $dependencies[]       = $this->categoriesID;
      }
    }
    if ($this->configuration['image_field']) {
      $process[$this->configuration['image_field']] = [
        'plugin' => 'migration',
        'migration' => $this->attachmentID,
        'source' => 'thumbnail_id',
      ];
      $dependencies[] = $this->attachmentID;
    }
    $migration->set('process', $process);
    if (!empty($this->authorID)) {
      $dependencies[] = $this->authorID;
    }
    $migration->set('migration_dependencies', ['required' => $dependencies]);
    $migration->save();

    // Also create a comment migration, if the content type has a comment field.
    /** @var \Drupal\Core\Field\FieldDefinitionInterface[] $all_fields */
    $all_fields = \Drupal::service('entity_field.manager')->getFieldDefinitions('node', $this->configuration[$wordpress_type]['type']);
    foreach ($all_fields as $field_name => $field_definition) {
      if ($field_definition->getType() == 'comment') {
        $storage   = $field_definition->getFieldStorageDefinition();
        $id        = $this->configuration['prefix'] . 'wordpress_comment_' . $wordpress_type;
        $migration = static::createEntityFromPlugin('wordpress_comment', $id);
        $migration->set('migration_group', $this->configuration['group_id']);
        $source                  = $migration->get('source');
        $source['item_selector'] = str_replace(':content_type', $wordpress_type, $source['item_selector']);
        $migration->set('source', $source);
        $process                                     = $migration->get('process');
        $process['entity_id'][0]['migration']        = $content_id;
        $process['comment_type'][0]['default_value'] = $storage->getSetting('comment_type');
        $process['pid'][0]['migration']              = $id;
        $process['field_name'][0]['default_value']   = $field_name;
        $migration->set('process', $process);
        $migration->set('migration_dependencies', ['required' => [$content_id]]);
        $migration->save();
        break;
      }
    }
  }

  /**
   * Returns the first field referencing a given vocabulary.
   *
   * @param string $bundle
   *   Name of bundle.
   * @param string $vocabulary
   *   Name of vocabulary.
   *
   * @return string
   *   Empty string.
   */
  protected function termField($bundle, $vocabulary) {
    /** @var \Drupal\Core\Field\FieldDefinitionInterface[] $all_fields */
    $all_fields = \Drupal::service('entity_field.manager')->getFieldDefinitions('node', $bundle);
    foreach ($all_fields as $field_name => $field_definition) {
      if ($field_definition->getType() == 'entity_reference') {
        $storage = $field_definition->getFieldStorageDefinition();
        if ($storage->getSetting('target_type') == 'taxonomy_term') {
          $handler_settings = $field_definition->getSetting('handler_settings');
          if (isset($handler_settings['target_bundles'][$vocabulary])) {
            return $field_name;
          }
        }
      }
    }
    return '';
  }

  /**
   * Create a configuration entity from a core migration plugin's configuration.
   *
   * @param string $plugin_id
   *   ID of a migration plugin managed by MigrationPluginManager.
   * @param string $new_plugin_id
   *   ID to use for the new configuration entity.
   *
   * @return \Drupal\Core\Entity\EntityInterface
   *   is same as \Drupal\migrate_plus\Entity\MigrationInterface
   *   A Migration configuration entity (not saved to persistent storage).
   *
   * @throws \Drupal\Component\Plugin\Exception\PluginException
   *
   * @todo: Remove and replace calls to use Migration::createEntityFromPlugin()
   * when there's a migrate_plus release containing it we can have a dependency
   * on.
   */
  protected static function createEntityFromPlugin($plugin_id, $new_plugin_id) {
    /** @var \Drupal\migrate\Plugin\MigrationPluginManagerInterface $plugin_manager */
    $plugin_manager                         = \Drupal::service('plugin.manager.migration');
    $migration_plugin                       = $plugin_manager->createInstance($plugin_id);
    $entity_array['id']                     = $new_plugin_id;
    $entity_array['migration_tags']         = $migration_plugin->get('migration_tags');
    $entity_array['label']                  = $migration_plugin->label();
    $entity_array['source']                 = $migration_plugin->getSourceConfiguration();
    $entity_array['destination']            = $migration_plugin->getDestinationConfiguration();
    $entity_array['process']                = $migration_plugin->getProcess();
    $entity_array['migration_dependencies'] = $migration_plugin->getMigrationDependencies();
    $migration_entity                       = Migration::create($entity_array);
    return $migration_entity;
  }

}
