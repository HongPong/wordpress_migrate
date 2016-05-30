<?php

namespace Drupal\wordpress_migrate\Wizard;

use Drupal\Core\Form\FormStateInterface;
use Drupal\ctools\Wizard\FormWizardBase;
use Drupal\migrate\Plugin\Migration as MigrationPlugin;
use Drupal\migrate_plus\Entity\Migration;
use Drupal\migrate_plus\Entity\MigrationGroup;

class ImportWizard extends FormWizardBase {

  /**
   * @var \Drupal\migrate\Plugin\Migration[]
   */
  protected $wordPressMigrations;

  /**
   * {@inheritdoc}
   */
  public function getOperations($cached_values) {
    $steps = [
      'source_select' => [
        'form' => 'Drupal\wordpress_migrate\Form\SourceSelectForm',
        'title' => $this->t('WordPress data source'),
      ],
      'authors' => [
        'form' => 'Drupal\wordpress_migrate\Form\AuthorForm',
        'title' => $this->t('Authors'),
      ],
      'content_select' => [
        'form' => 'Drupal\wordpress_migrate\Form\ContentSelectForm',
        'title' => $this->t('Select content'),
      ],
    ];
    if (!empty($cached_values['blog_post_type'])) {
      $steps += [
        'blog_post' => [
          'form' => 'Drupal\wordpress_migrate\Form\ContentTypeForm',
          'title' => $this->t('Blog Posts'),
          'values' => ['wordpress_content_type' => 'post'],
        ],
      ];
    }
    if (!empty($cached_values['page_type'])) {
      $steps += [
        'page' => [
          'form' => 'Drupal\wordpress_migrate\Form\ContentTypeForm',
          'title' => $this->t('Pages'),
          'values' => ['wordpress_content_type' => 'page'],
        ],
      ];
    }
    $steps += [
      'review' => [
        'form' => 'Drupal\wordpress_migrate\Form\ReviewForm',
        'title' => $this->t('Review'),
      ],
    ];
    return $steps;
  }

  /**
   * {@inheritdoc}
   */
  public function getRouteName() {
    return 'wordpress_migrate.wizard.import.step';
  }

  /**
   * {@inheritdoc}
   */
  public function finish(array &$form, FormStateInterface $form_state) {
    $cached_values = $form_state->getTemporaryValue('wizard');
    dpm($cached_values);
    // Create the migration group.
    // @todo: Allow group ID/prefix to be customized.
    $group_id = 'my_wordpress';
    $group_configuration = [
      'id' => $group_id,
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
            $cached_values['file_uri'],
          ],
        ],
      ],
    ];
    MigrationGroup::create($group_configuration)->save();
    $plugin_manager = \Drupal::service('plugin.manager.migration');
    /** @var \Drupal\migrate\Plugin\Migration[] $all_migrations */
    $this->wordPressMigrations = $plugin_manager->createInstancesByTag('WordPress');
    if ($cached_values['blog_post_type']) {
      $this->createContentMigration('post', $cached_values['blog_post_type']);
    }
    if ($cached_values['page_type']) {
      $this->createContentMigration('page', $cached_values['page_type']);
    }

    parent::finish($form, $form_state);
  }

  protected function createContentMigration($wordpress_content_type, $drupal_content_type) {
    $content_migration = $this->wordPressMigrations['wordpress_content'];
    $entity_array['id'] = 'my_wordpress_content_' . $wordpress_content_type;
    $entity_array['migration_group'] = 'my_wordpress';
    $entity_array['migration_tags'] = $content_migration->get('migration_tags');
    $entity_array['label'] = $content_migration->get('label');
    $entity_array['source'] = $content_migration->getSourceConfiguration();
    $entity_array['source']['item_selector'] .= '[wp:post_type="' . $wordpress_content_type . '"]"';
    $entity_array['destination'] = $content_migration->getDestinationConfiguration();
    $entity_array['process'] = $content_migration->getProcess();
    $entity_array['process']['type'] = [
      'plugin' => 'default_value',
      'default_value' => $drupal_content_type,
    ];
    $entity_array['process']['body/format'] = 'full_html';
    $entity_array['migration_dependencies'] = $content_migration->getMigrationDependencies();
    $migration_entity = Migration::create($entity_array);
    $migration_entity->save();
  }

}
