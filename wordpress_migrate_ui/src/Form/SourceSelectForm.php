<?php

namespace Drupal\wordpress_migrate_ui\Form;

use Drupal\Core\Entity\EntityStorageException;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Component\Utility\Environment;

/**
 * Simple wizard step form.
 */
class SourceSelectForm extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'wordpress_migrate_source_select_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    // @todo Make sure we have a private directory configured.
    // @link https://www.drupal.org/node/2742291
    // @todo Support importing directly from WP admin interface.
    // @link https://www.drupal.org/node/2742293
    // @todo Support importing from the directory directly.
    // @link https://www.drupal.org/node/2742295
    // @todo Support importing from a database.
    // @link https://www.drupal.org/node/2742299
    $form['overview'] = [
      '#markup' => $this->t('This wizard supports importing into your Drupal site from a WordPress blog. To be able to use this wizard, you must have an XML file exported from the blog.'),
    ];
    $form['description'] = [
      '#markup' => $this->t('<br /><br />You will be led through a series of steps, allowing you to customize what will be imported into Drupal and how it will be mapped. At the end of this process, a migration group will be generated.'),
    ];
    $form['wxr_file'] = [
      '#type' => 'file',
      '#title' => $this->t('WordPress exported file (WXR)'),
      '#description' => $this->t('Select an exported WordPress file (.xml extension). Maximum file size is @size.',
        ['@size' => format_size(Environment::getUploadMaxSize())]),
    ];
    $form['keep_wxr_file'] = [
      '#type' => 'checkbox',
      '#title' => $this->t('Permanently save uploaded WXR file'),
      '#description' => $this->t('The uploaded WXR file will be kept in your site permanently. It will always be visible on the "Files" section of the Content administration area.'),
    ];
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state) {
    $all_files = $this->getRequest()->files->get('files', []);
    if (empty($all_files['wxr_file'])) {
      $form_state->setErrorByName('wxr_file', $this->t('You must upload a file to continue.'));
    }
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $validators = ['file_validate_extensions' => ['xml']];
    // file_save_upload renames if file already exists (default behavior)
    $file = file_save_upload('wxr_file', $validators, 'public://', 0);
    if ($file) {
      $cached_values = $form_state->getTemporaryValue('wizard');
      $cached_values['file_uri'] = $file->getFileUri();
      if ($form_state->getValue('keep_wxr_file')) {
        /* Set the status flag permanent of the file object */
        $file->setPermanent();
        /* Save the file in database */
        try {
          $file->save();
        }
        catch (EntityStorageException $e) {
          $this->messenger()->addError('The WXR file failed to upload. Please try again.');
          $this->logger('wordpress_migrate')->error('The WXR file failed to upload. (EntityStorageException)');
        }
      }
      $form_state->setTemporaryValue('wizard', $cached_values);
      // @todo: Preprocess the file
      // @link https://www.drupal.org/node/2742301
    }
    else {
      $form_state->setRebuild();
      $this->messenger()->addError($this->t('WXR file upload failed. Please try again. Your log messages may have additional information.'));
      $this->logger('wordpress_migrate')->error('The WXR file failed to upload.');
    }
  }

}
