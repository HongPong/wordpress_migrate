<?php

namespace Drupal\wordpress_migrate_ui\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\StringTranslation\StringTranslationTrait;

/**
 * Simple wizard step form.
 */
class ReviewForm extends FormBase {

  use StringTranslationTrait;

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
    ];
    $form['prefix'] = [
      '#type' => 'machine_name',
      '#max_length' => 64 - strlen('wordpress_content_page'),
      '#title' => $this->t('ID to prepend to each generated migration'),
      '#default_value' => 'my_',
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

}
