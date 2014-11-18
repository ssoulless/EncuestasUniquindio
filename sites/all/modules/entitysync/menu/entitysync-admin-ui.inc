<?php
/**
 * @file
 * Entity Sync administration UI related functions.
 */

/**
 * Entity Sync administration form.
 */
function entitysync_admin_form() {

  $form = array();

  $form['security'] = array(
    '#type' => 'fieldset',
    '#title' => t('Security'),
    '#collapsible' => FALSE,
  );

  $form['errorhandling'] = array(
    '#type' => 'fieldset',
    '#title' => t('Error handling'),
    '#collapsible' => FALSE,
  );

  $form['security']['entitysync_shared_secret'] = array(
    '#type' => 'textfield',
    '#title' => t('Shared secret between sites'),
    '#description' => t('This is the shared secret between synchronizing Drupal sites. This must be the same in synchronized sites.'),
    '#size' => 128,
    '#required' => TRUE,
    '#default_value' => variable_get('entitysync_shared_secret', 'undefined'),
  );

  $form['errorhandling']['entitysync_error_limit'] = array(
    '#type' => 'textfield',
    '#title' => t('Retries'),
    '#description' => t('How many times a packet is resent to the host after it has failed for the first time. Settings this to 0 completely disables the queue.'),
    '#size' => 128,
    '#required' => TRUE,
    '#default_value' => variable_get('entitysync_error_limit', '5'),
  );

  $form['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}

/**
 * Entity Sync settings form submit handler.
 */
function entitysync_admin_form_submit($form_id, $form_state) {
  variable_set('entitysync_shared_secret', $form_state['values']['entitysync_shared_secret']);
  variable_set('entitysync_error_limit', $form_state['values']['entitysync_error_limit']);
}
