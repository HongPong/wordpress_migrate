<?php

namespace Drupal\wordpress_migrate_ui\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;

/**
 * Simple wizard step form.
 */
class ContentSelectForm extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'wordpress_migrate_content_select_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    // Start clean in case we came here via Previous.
    $cached_values = $form_state->getTemporaryValue('wizard');
    unset($cached_values['post']);
    unset($cached_values['page']);
    $form_state->setTemporaryValue('wizard', $cached_values);

    $form['overview'] = [
      '#markup' => $this->t('WordPress blogs contain two primary kinds of content, blog posts and pages. Here you may choose what types of Drupal nodes to create from each of those content types, or omit one or both from the import entirely.'),
    ];

    // Get destination node type(s)
    $node_types = node_type_get_types();
    $options = ['' => $this->t('Do not import')];
    foreach ($node_types as $node_type => $info) {
      $options[$node_type] = $info->get('name');
    }

    $form['blog_post_type'] = [
      '#type' => 'select',
      '#title' => $this->t('Import WordPress blog posts as'),
      '#options' => $options,
    ];

    $form['page_type'] = [
      '#type' => 'select',
      '#title' => $this->t('Import WordPress pages as'),
      '#options' => $options,
    ];

    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state) {
    if (!$form_state->getValue('blog_post_type') && $form_state->getValue('page_type')) {
      $form_state->setErrorByName('', $this->t('Please select at least one content type to import.'));
    }
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $cached_values = $form_state->getTemporaryValue('wizard');
    $cached_values['post']['type'] = $form_state->getValue('blog_post_type');
    $cached_values['page']['type'] = $form_state->getValue('page_type');
    $form_state->setTemporaryValue('wizard', $cached_values);
  }

}
