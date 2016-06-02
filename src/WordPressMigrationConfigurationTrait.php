<?php

namespace Drupal\wordpress_migrate;

use Drupal\migrate_plus\Entity\MigrationGroup;

/**
 * Functionality to construct WordPress migrations from broad configuration
 * settings.
 */
trait WordPressMigrationConfigurationTrait {

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
   * Creates a set of WordPress import migrations based on configuration settings.
   * 
   * @param $configuration
   *   An associative array:
   *   - file_uri: Drupal stream wrapper of the source WordPress XML file.
   *   - group_id: ID of the MigrationGroup holding the generated migrations.
   *   - prefix: String to prefix to the IDs of generated migrations.
   *   - default_author_uid: If present, uid to author all imported content. If
   *     absent or empty, users will be imported from WordPress.
   *   - tag_vocabulary: Machine name of vocabulary to hold tags.
   *   - category_vocabulary: Machine name of vocabulary to hold categories.
   *   - [post|page]: Associative array of type-specific configuration:
   *     - drupal_type: Machine name of Drupal node bundle to hold content.
   *     - text_format: Machine name of text format for body field.
   */
  protected function createMigrations(array $configuration) {
    $this->configuration = $configuration;
    // Create the migration group.
    $group_configuration = [
      'id' => $configuration['group_id'],
      // @todo: Add Wordpress site title in here.
      'label' => 'Imports from WordPress site',
      'source_type' => 'WordPress',
      'shared_configuration' => [
        'source' => [
          // @todo: Dynamically populate from the source XML.
          'namespaces' => [
            'wp' => 'http://wordpress.org/export/1.2/',
            'excerpt' => 'http://wordpress.org/export/1.2/excerpt/',
            'content' => 'http://purl.org/rss/1.0/modules/content/',
            'wfw' => 'http://wellformedweb.org/CommentAPI/',
            'dc' => 'http://purl.org/dc/elements/1.1/',
          ],
          'urls' => [
            $configuration['file_uri'],
          ],
        ],
      ],
    ];
    MigrationGroup::create($group_configuration)->save();
    if ($configuration['default_author_uid']) {
      $this->uidMapping = [
        'plugin' => 'default_value',
        'default_value' => $configuration['default_author_uid'],
      ];
    }
    else {
      $author_id = $configuration['prefix'] . 'wordpress_authors';
      $migration = static::createEntityFromPlugin('wordpress_authors', $author_id);
      $migration->set('migration_group', $configuration['group_id']);
      $migration->save();
      $this->uidMapping = [
        'plugin' => 'migration',
        'migration' => $author_id,
        'source' => 'creator',
      ];
    }
    if ($configuration['tag_vocabulary']) {
      $this->tagsID = $configuration['prefix'] . 'wordpress_tags';
      $migration = static::createEntityFromPlugin('wordpress_tags', $this->tagsID);
      $migration->set('migration_group', $configuration['group_id']);
      $process = $migration->get('process');
      $process['vid'] = [
        'plugin' => 'default_value',
        'default_value' => $configuration['tag_vocabulary']
      ];
      $migration->set('process', $process);
      $migration->save();
    }
    if ($configuration['category_vocabulary']) {
      $this->categoriesID = $configuration['prefix'] . 'wordpress_categories';
      $migration = static::createEntityFromPlugin('wordpress_categories', $this->categoriesID);
      $migration->set('migration_group', $configuration['group_id']);
      $process = $migration->get('process');
      $process['vid'] = [
        'plugin' => 'default_value',
        'default_value' => $configuration['category_vocabulary']
      ];
      $migration->set('process', $process);
      $migration->save();
    }
    foreach (['post', 'page'] as $wordpress_type) {
      $this->createContentMigration($wordpress_type);
    }
  }

  protected function createContentMigration($wordpress_type) {
    $dependencies = [];
    $id = $this->configuration['prefix'] . 'wordpress_content_' . $wordpress_type;
    $migration = static::createEntityFromPlugin('wordpress_content', $id);
    $migration->set('migration_group', $this->configuration['group_id']);
    $source = $migration->get('source');
    $source['item_selector'] .= '[wp:post_type="' . $wordpress_type . '"]';
    $migration->set('source', $source);
    $process = $migration->get('process');
    $process['uid'] = $this->uidMapping;
    $process['body/format'] = [
      'plugin' => 'default_value',
      'default_value' => $this->configuration[$wordpress_type]['text_format'],
    ];
    $process['type'] = [
      'plugin' => 'default_value',
      'default_value' => $this->configuration[$wordpress_type]['type'],
    ];
    if ($this->configuration['tag_vocabulary']) {
      if ($term_field = $this->termField($this->configuration[$wordpress_type]['type'], $this->configuration['tag_vocabulary'])) {
        $process[$term_field] = [
          'plugin' => 'migration',
          'migration' => $this->tagsID,
          'source' => 'post_tag',
        ];
        $dependencies[] = $this->tagsID;
      }
    }
    if ($this->configuration['category_vocabulary']) {
      if ($term_field = $this->termField($this->configuration[$wordpress_type]['type'], $this->configuration['category_vocabulary'])) {
        $process[$term_field] = [
          'plugin' => 'migration',
          'migration' => $this->categoriesID,
          'source' => 'category',
        ];
        $dependencies[] = $this->categoriesID;
      }
    }
    $migration->set('process', $process);
    if (isset($author_id)) {
      $dependencies[] = $author_id;
    }
    $migration->set('migration_dependencies', ['required' => $dependencies]);
    $migration->save();
  }

  /**
   * Returns the first field referencing a given vocabulary.
   *
   * @param string $bundle
   * @param string $vocabulary
   * @return string
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
   * @todo: Remove and replace calls to use Migration::createEntityFromPlugin()
   * when there's a migrate_plus release containing it we can have a dependency
   * on.
   *
   * @param string $plugin_id
   *   ID of a migration plugin managed by MigrationPluginManager.
   * @param string $new_plugin_id
   *   ID to use for the new configuration entity.
   *
   * @return \Drupal\migrate_plus\Entity\MigrationInterface
   *   A Migration configuration entity (not saved to persistent storage).
   */
  protected static function createEntityFromPlugin($plugin_id, $new_plugin_id) {
    /** @var \Drupal\migrate\Plugin\MigrationPluginManagerInterface $plugin_manager */
    $plugin_manager = \Drupal::service('plugin.manager.migration');
    $migration_plugin = $plugin_manager->createInstance($plugin_id);
    $entity_array['id'] = $new_plugin_id;
    $entity_array['migration_tags'] = $migration_plugin->get('migration_tags');
    $entity_array['label'] = $migration_plugin->label();
    $entity_array['source'] = $migration_plugin->getSourceConfiguration();
    $entity_array['destination'] = $migration_plugin->getDestinationConfiguration();
    $entity_array['process'] = $migration_plugin->getProcess();
    $entity_array['migration_dependencies'] = $migration_plugin->getMigrationDependencies();
    $migration_entity = static::create($entity_array);
    return $migration_entity;
  }

}
