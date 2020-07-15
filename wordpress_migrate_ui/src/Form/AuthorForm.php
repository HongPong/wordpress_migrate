<?php

namespace Drupal\wordpress_migrate_ui\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\StringTranslation\StringTranslationTrait;

/**
 * Simple wizard step form.
 */
class AuthorForm extends FormBase {

  use StringTranslationTrait;

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'wordpress_migrate_author_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    $form['overview'] = [
      '#markup' => $this->t('User accounts for authors in the WordPress blog may be imported to Drupal. If you select <strong>Yes</strong>, any authors in the WordPress file who do not match existing Drupal accounts (based on email address) will have new Drupal accounts automatically created. Note that their passwords are not imported - they must be reset after import.<br/>If you select <strong>No</strong>, you must choose an existing Drupal account which will be the author of any WordPress content whose author is not imported.'),
    ];

    $form['perform_user_migration'] = [
      '#type' => 'radios',
      '#title' => $this->t('Create new users for existing WordPress content authors?'),
      '#options' => [1 => $this->t('Yes'), 0 => $this->t('No')],
      '#default_value' => 1,
    ];

    $form['default_author'] = [
      '#type' => 'textfield',
      '#title' => $this->t('Username of default content author:'),
      '#default_value' => $this->currentUser()->getAccountName(),
      '#autocomplete_path' => 'user/autocomplete',
      '#states' => [
        'invisible' => [
          'input[name="perform_user_migration"]' => ['value' => 1],
        ],
      ],
    ];
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state) {
    if (!$form_state->getValue('perform_user_migration')) {
      $account = user_load_by_name($form_state->getValue('default_author'));
      if (!$account) {
        $form_state->setErrorByName('default_author_uid', $this->t('@name is not a valid username',
          ['@name' => $form_state->getValue('default_author')]));
      }
    }
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $cached_values = $form_state->getTemporaryValue('wizard');
    if (!$form_state->getValue('perform_user_migration')) {
      $account = user_load_by_name($form_state->getValue('default_author'));
      if ($account) {
        $cached_values['default_author'] = $form_state->getValue('default_author');
      }
    }
    $form_state->setTemporaryValue('wizard', $cached_values);
  }

}
