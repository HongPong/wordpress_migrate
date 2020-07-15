<?php

namespace Drupal\wordpress_migrate_ui\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;

/**
 * Simple wizard step form.
 */
class ImageSelectForm extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'wordpress_migrate_image_select_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    // Start clean in case we came here via Previous.
    $cached_values = $form_state->getTemporaryValue('wizard');
    unset($cached_values['image_field']);
    $form_state->setTemporaryValue('wizard', $cached_values);

    $form['overview'] = [
      '#markup' => $this->t('Here you may choose the Drupal image field to import Wordpress featured images into.'),
    ];

    // @todo this should be dependency injection.
    $field_map = \Drupal::service('entity_field.manager')->getFieldMap();
    $options = ['' => $this->t('Do not import')];
    foreach($field_map as $entity_type => $fields) {
      if ($entity_type == 'node') {
        foreach($fields as $field_name => $field_settings) {
          if ($field_settings['type'] == 'image') {
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
  public function validateForm(array &$form, FormStateInterface $form_state) {
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $cached_values = $form_state->getTemporaryValue('wizard');
    $cached_values['image_field'] = $form_state->getValue('image_field');
    $form_state->setTemporaryValue('wizard', $cached_values);
  }

}
