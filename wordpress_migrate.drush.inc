<?php

/**
 * @file
 * Command-line tools for managing WordPress migrations.
 */

use Drupal\wordpress_migrate\WordPressMigrationGenerator;

/**
 * Implements hook_drush_command().
 */
function wordpress_migrate_drush_command() {
  $items['wordpress-migrate-generate'] = [
    'description' => 'Generate migrations for a WordPress file.',
    'callback' => 'drush_wordpress_migrate_generate',
    'options' => [
      'group-id' => [
        'description' => 'ID of the migration group to create.',
        'required' => TRUE,
        'example-value' => 'my_wordpress_import',
      ],
      'prefix' => [
        'description' => 'String to prefix to the IDs of generated migrations.',
        'example-value' => 'my_',
      ],
      'default-author' => [
        'description' => 'If present, username to author all imported content. If omitted, users will be imported from WordPress.',
        'example-value' => 'author_account',
      ],
      'tag-vocabulary' => [
        'description' => 'Machine name of vocabulary to hold imported tags.',
        'example-value' => 'tags',
      ],
      'category-vocabulary' => [
        'description' => 'Machine name of vocabulary to hold imported categories.',
        'example-value' => 'categories',
      ],
      'post-type' => [
        'description' => 'Machine name of Drupal node bundle to hold imported post content.',
        'example-value' => 'blog',
      ],
      'post-text-format' => [
        'description' => 'Machine name of text format for body field on imported post content.',
        'example-value' => 'restricted_html',
      ],
      'page-type' => [
        'description' => 'Machine name of Drupal node bundle to hold imported page content.',
        'example-value' => 'blog',
      ],
      'page-text-format' => [
        'description' => 'Machine name of text format for body field on imported page content.',
        'example-value' => 'restricted_html',
      ],
    ],
    'arguments' => [
      'file_uri' => 'Address of the WordPress export file to migrate into Drupal - either a Drupal stream wrapper (private://my_wp_export.xml) or an absolute filespec on the system drush is running on (/var/data/my_wp_export.sml).',
    ],
    'required-arguments' => TRUE,
    'examples' => [
      'wordpress-migrate-generate /var/data/my_wp_export.xml --group-id=old_blog --prefix=blog_ --tag-vocabulary=tags --post-type=article --post-text-format=restricted_html --page-type=page --page-text-format=full_html' => 'Set up to import all content except categories from a WordPress file.',
    ],
    'drupal dependencies' => ['wordpress_migrate'],
  ];

  return $items;
}

/**
 * Generate WordPress migrations based on the command-line options.
 *
 * @param string $file_uri
 *   The url of the file.
 */
function drush_wordpress_migrate_generate($file_uri) {
  $configuration['file_uri'] = $file_uri;
  $configuration['group_id'] = drush_get_option('group-id');
  $configuration['prefix'] = drush_get_option('prefix', '');
  $configuration['default_author'] = drush_get_option('default-author', '');
  $configuration['tag_vocabulary'] = drush_get_option('tag-vocabulary', '');
  $configuration['category_vocabulary'] = drush_get_option('category-vocabulary', '');
  $configuration['post']['type'] = drush_get_option('post-type', '');
  $configuration['post']['text_format'] = drush_get_option('post-text-format', '');
  $configuration['page']['type'] = drush_get_option('page-type', '');
  $configuration['page']['text_format'] = drush_get_option('page-text-format', '');

  $generator = new WordPressMigrationGenerator($configuration);
  $generator->createMigrations();

  $table = [];
  $migrations = _wordpress_migrate_migration_list($configuration['group_id']);
  // Take it one group at a time, listing the migrations within each group.
  foreach ($migrations as $migration_id => $migration) {
    $table[] = [$migration_id];
  }
  drush_print(dt('The following migrations have been generated:'));
  drush_print_table($table);
}

/**
 * Retrieve a list of migrations in the specified group.
 *
 * @param string $group_id
 *   Group machine name.
 *
 * @return \Drupal\migrate\Plugin\MigrationInterface[]
 *   List of migrations in the given group.
 */
function _wordpress_migrate_migration_list($group_id) {
  /** @var \Drupal\migrate_plus\Plugin\MigrationConfigEntityPluginManager $manager */
  $manager = \Drupal::service('plugin.manager.migration');
  $plugins = $manager->createInstances([]);
  $migrations = [];
  foreach ($plugins as $id => $migration) {
    if ($group_id == $migration->get('migration_group')) {
      $migrations[$id] = $migration;
    }
  }
  return $migrations;
}
