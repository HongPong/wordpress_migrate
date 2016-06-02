<?php

namespace Drupal\wordpress_migrate_ui\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\taxonomy\Entity\Vocabulary;

/**
 * Simple wizard step form.
 */
class VocabularySelectForm extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'wordpress_migrate_vocabulary_select_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    // Start clean in case we came here via Previous.
    $cached_values = $form_state->getTemporaryValue('wizard');
    unset($cached_values['tag_vocabulary']);
    unset($cached_values['category_vocabulary']);
    $form_state->setTemporaryValue('wizard', $cached_values);

    $form['overview'] = [
      '#markup' => $this->t('WordPress blogs contain two vocabularies, tags and categories. Here you may choose the Drupal vocabularies to import each into, or omit one or both from the import entirely.'),
    ];

    // Get destination node type(s)
    $vocabularies = Vocabulary::loadMultiple();
    $options = ['' => $this->t('Do not import')];
    foreach ($vocabularies as $vocabulary_id => $info) {
      $options[$vocabulary_id] = $info->label();
    }

    $form['tag_vocabulary'] = [
      '#type' => 'select',
      '#title' => $this->t('Import WordPress tags as'),
      '#options' => $options,
    ];

    $form['category_vocabulary'] = [
      '#type' => 'select',
      '#title' => $this->t('Import WordPress categories as'),
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
    $cached_values['tag_vocabulary'] = $form_state->getValue('tag_vocabulary');
    $cached_values['category_vocabulary'] = $form_state->getValue('category_vocabulary');
    $form_state->setTemporaryValue('wizard', $cached_values);
  }

}
