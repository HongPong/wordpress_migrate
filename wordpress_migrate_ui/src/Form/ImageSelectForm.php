<?php

namespace Drupal\wordpress_migrate_ui\Form;

use Drupal\Core\Entity\EntityFieldManagerInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Simple wizard step form.
 */
class ImageSelectForm extends FormBase {

  /**
   * Constructs a new ImageSelectForm.
   *
   * @param \Drupal\Core\Entity\EntityFieldManagerInterface $entityFieldManager
   *   The entity field manager.
   */
  public function __construct(
    protected readonly EntityFieldManagerInterface $entityFieldManager,
  ) {
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container) {
    return new static(
      $container->get('entity_field.manager')
    );
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'wordpress_migrate_image_select_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state): array {
    // Start clean in case we came here via Previous.
    $cached_values = $form_state->getTemporaryValue('wizard');
    unset($cached_values['image_field']);
    $form_state->setTemporaryValue('wizard', $cached_values);

    $form['overview'] = [
      '#markup' => $this->t('Here you may choose the Drupal image field to import Wordpress featured images into.'),
    ];

    $field_map = $this->entityFieldManager->getFieldMap();
    $options = ['' => $this->t('Do not import')];
    foreach ($field_map as $entity_type => $fields) {
      if ($entity_type === 'node') {
        foreach ($fields as $field_name => $field_settings) {
          if ($field_settings['type'] === 'image') {
            $options[$field_name] = $field_name;
          }
        }
      }
    }

    $form['image_field'] = [
      '#type' => 'select',
      '#title' => $this->t('Import WordPress featured images in'),
      '#options' => $options,
    ];

    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state): void {
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state): void {
    $cached_values = $form_state->getTemporaryValue('wizard');
    $cached_values['image_field'] = $form_state->getValue('image_field');
    $form_state->setTemporaryValue('wizard', $cached_values);
  }

}
