<?php

namespace Drupal\wordpress_migrate_ui\Form;

use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\StringTranslation\StringTranslationTrait;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Simple wizard step form.
 */
class ReviewForm extends FormBase {

  use StringTranslationTrait;

  /**
   * The entity type manager.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected $entityTypeManager;

  /**
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entity_type_manager
   *   The entity type manager.
   */
  public function __construct(EntityTypeManagerInterface $entity_type_manager) {
    $this->entityTypeManager = $entity_type_manager;
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container) {
    return new static($container->get('entity_type.manager'));
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'wordpress_migrate_review_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    // @todo: Display details of the configuration.
    // @link: https://www.drupal.org/node/2742289
    $form['description'] = [
      '#markup' => $this->t('Please review your migration configuration. When you submit this form, migration processes will be created and you will be left at the migration dashboard.'),
    ];
    // @todo: Derive default values from blog title.
    // @link https://www.drupal.org/node/2742287
    $form['group_id'] = [
      '#type' => 'machine_name',
      '#max_length' => 64,
      '#title' => $this->t('ID to assign to the generated migration group'),
      '#default_value' => 'my_wordpress',
      '#machine_name' => [
        'exists' => [$this, 'groupExists'],
      ],
    ];
    $form['prefix'] = [
      '#type' => 'machine_name',
      '#max_length' => 64 - strlen('wordpress_content_page'),
      '#title' => $this->t('ID to prepend to each generated migration'),
      '#default_value' => 'my_',
      '#machine_name' => [
        'exists' => [$this, 'prefixExists'],
      ],
    ];

    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $cached_values = $form_state->getTemporaryValue('wizard');
    $cached_values['group_id'] = $form_state->getValue('group_id');
    $cached_values['prefix'] = $form_state->getValue('prefix');
    $form_state->setTemporaryValue('wizard', $cached_values);
  }

  /**
   * Determines if the migration group already exists.
   *
   * @param string $id
   *   The migration group ID
   *
   * @return bool
   *   TRUE if the migration group exists, FALSE otherwise.
   */
  public function groupExists($id) {
    return (bool) $this->entityTypeManager
      ->getStorage('migration_group')
      ->getQuery()
      ->condition('id', $id)
      ->execute();
  }

  /**
   * Determines if the migration with prefix already exists.
   *
   * @param string $prefix
   *   The migration prefix
   *
   * @return bool
   *   TRUE if the migration with prefix exists, FALSE otherwise.
   */
  public function prefixExists($prefix) {
    return (bool) $this->entityTypeManager
      ->getStorage('migration')
      ->getQuery()
      ->condition('id', $prefix . 'wordpress_attachments')
      ->execute();
  }

}
