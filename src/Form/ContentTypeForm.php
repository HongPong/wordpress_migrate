<?php

namespace Drupal\wordpress_migrate\Form;

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
    // Start clean in case we came here via Previous.
    $cached_values = $form_state->getTemporaryValue('wizard');
    $content_type = $cached_values['wordpress_content_type'];
    switch ($content_type) {
      case 'post':
        $selected_post_type = $cached_values['blog_post_type'];
        break;
      case 'page':
        $selected_post_type = $cached_values['page_type'];
        break;
    }

    $empty_field = [
      '#type' => 'value',
      '#value' => '',
    ];
/*
    $vocabs = $this->vocabularies($selected_post_type);
    if (!empty($vocabs)) {
      $options = array('' => t('Do not import'));
      foreach ($vocabs as $machine_name => $name) {
        $options[$machine_name] = $name;
      }

      // If field_tags exists, default to it.
      $tags_default = (isset($options['field_tags']) ? 'field_tags' : '');
      $form['tag_field'] = array(
        '#type' => 'select',
        '#title' => t('Import WordPress tags to the term reference field'),
        '#default_value' => $tags_default,
        '#options' => $options,
      );

      $form['category_field'] = array(
        '#type' => 'select',
        '#title' => t('Import WordPress categories to the term reference field'),
        '#default_value' => '',
        '#options' => $options,
      );
    }
    else {
      $form['tag_field'] = $form['category_field'] = $empty_field;
    }
*/
    /*
    if (module_exists('comment') &&
      (variable_get('comment_' . $selected_post_type, COMMENT_NODE_OPEN)
        != COMMENT_NODE_CLOSED)) {
      $form['comments'] = array(
        '#type' => 'radios',
        '#title' => t('Import comments?'),
        '#options' => array(1 => t('Yes'), 0 => t('No')),
        '#default_value' => 1,
      );
      $form['pingbacks'] = array(
        '#type' => 'radios',
        '#title' => t('Ignore pingbacks?'),
        '#options' => array(1 => t('Yes'), 0 => t('No')),
        '#default_value' => 1,
      );
    }
    else {
      $form['comments'] = array(
        '#type' => 'value',
        '#value' => 0,
      );
      $form['pingbacks'] = array(
        '#type' => 'value',
        '#value' => 0,
      );
    }
*/
    /*
    $file_fields = array('' => t('Do not import'));
    $file_fields += $this->fileFields($selected_post_type, 'file')
      + $this->fileFields($selected_post_type, 'media');
    $file_image_fields = $file_fields +
      $this->fileFields($selected_post_type, 'image');
    if (count($file_image_fields) > 1) {
      $form['attachment_field'] = array(
        '#type' => 'select',
        '#title' => t('Field for attachments (including images)'),
        '#default_value' => '',
        '#options' => $file_image_fields,
        '#description' => t(''),
        '#states' => array(
          'visible' => array(
            'input[name="destination_type"]' => array('value' => 'blog'),
          ),
        ),
      );
    }
    else {
      $form['attachment_field'] = $empty_field;
    }
*/
    $options = [];
    foreach (filter_formats() as $format_id => $format) {
      $options[$format_id] = $format->get('name');
    }
    $form['text_format'] = [
      '#type' => 'select',
      '#title' => $this->t('Default format for text fields'),
      '#default_value' => array_key_exists('filtered_html', $options) ? 'filtered_html' : NULL,
      '#options' => $options,
      '#description' => t(''),
    ];
/*    $form['text_format_comment'] = array(
      '#type' => 'select',
      '#title' => t('Default format for comment text fields'),
      '#default_value' => 'filtered_html',
      '#options' => $options,
      '#description' => t(''),
      '#states' => array(
        'invisible' => array(
          'input[name="comments"]' => array('value' => 0),
        ),
      ),
    );
*/
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
    $content_type = $cached_values['wordpress_content_type'];
    $cached_values[$content_type . '_text_format'] = $form_state->getValue('text_format');
    $form_state->setTemporaryValue('wizard', $cached_values);
  }

}
