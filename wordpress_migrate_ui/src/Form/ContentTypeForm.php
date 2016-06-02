<?php

namespace Drupal\wordpress_migrate_ui\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;

/**
 * Simple wizard step form.
 */
class ContentTypeForm extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'wordpress_migrate_content_type_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    $options = [];
    foreach (filter_formats() as $format_id => $format) {
      $options[$format_id] = $format->get('name');
    }
    $form['text_format'] = [
      '#type' => 'select',
      '#title' => $this->t('Default format for text fields'),
      '#default_value' => array_key_exists('filtered_html', $options) ? 'filtered_html' : NULL,
      '#options' => $options,
    ];
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $cached_values = $form_state->getTemporaryValue('wizard');
    $content_type = $cached_values['wordpress_content_type'];
    $cached_values[$content_type]['text_format'] = $form_state->getValue('text_format');
    $form_state->setTemporaryValue('wizard', $cached_values);
  }

}
