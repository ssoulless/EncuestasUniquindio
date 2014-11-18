<?php
/**
 * @file
 * Entity Sync Rules related functions.
 */

/**
 * Implements hook_rules_action_info().
 */
function entitysync_rules_action_info() {
  $defaults = array(
    'group' => t('Entity Sync'),
    'access callback' => 'entitysync_access_callback',
  );

  $items['entitysync_sync'] = $defaults + array(
    'label' => t('Synchronize entity'),
    'base' => 'entitysync_rules_sync',
    'parameter' => array(
      'entity' => array(
        'type' => '*',
        'label' => t('Entity'),
        'description' => t('Select an entity to synchronize.'),
        'restriction' => 'selector',
        'wrapped' => TRUE,
        'allow null' => FALSE,
      ),
      'unique' => array(
        'type' => '*',
        'label' => t('Entity identifier'),
        'description' => t('Select a unique aspect (e.g. mail, uuid) which identifies the entity on both ends.'),
        'restriction' => 'selector',
        'wrapped' => TRUE,
        'allow null' => FALSE,
      ),
      'fields' => array(
        'type' => 'list<text>',
        'label' => t('Aspects to synchronize'),
        'description' => t('Select aspects which are synchronized to the endpoint.'),
        'options list' => '_entitysync_actions_get_entity_fields',
      ),
      'create' => array(
        'type' => 'boolean',
        'label' => t('Create entity on endpoint'),
        'description' => t('Check this if you want to create the entity when it does not exist on the endpoint.'),
        'restriction' => 'input',
        'wrapped' => TRUE,
        'allow null' => FALSE,
      ),
      'endpoint' => array(
        'type' => 'text',
        'label' => t('Endpoint URL'),
        'description' => t('URL of the endpoints callback e.g. https://www.example.com/entitysync. You can add multiple endpoints one per line.'),
        'restriction' => 'input',
        'wrapped' => TRUE,
        'allow null' => FALSE,
      ),
    ),
    'callbacks' => array(
      //'access' => 'rules_action_entity_createfetch_access',
      'form_alter' => '_entitysync_action_form_alter',
      //'validate' => '_entitysync_action_form_validate',
    ),
  );

  $items['entitysync_del'] = $defaults + array(
    'label' => t('Delete synchronized entity'),
    'base' => 'entitysync_rules_del',
    'parameter' => array(
      'entity' => array(
        'type' => '*',
        'label' => t('Entity'),
        'description' => t('Select an entity to delete on the endpoint.'),
        'restriction' => 'selector',
        'wrapped' => TRUE,
        'allow null' => FALSE,
      ),
      'unique' => array(
        'type' => '*',
        'label' => t('Entity identifier'),
        'description' => t('Select a unique aspect (e.g. mail, uuid) which identifies the entity on both ends.'),
        'restriction' => 'selector',
        'wrapped' => TRUE,
        'allow null' => FALSE,
      ),
      'endpoint' => array(
        'type' => 'text',
        'label' => t('Endpoint URL'),
        'description' => t('URL of the endpoints callback e.g. https://www.example.com/entitysync. You can add multiple endpoints.'),
        'restriction' => 'input',
        'wrapped' => TRUE,
        'allow null' => FALSE,
      ),
    ),
  );

  return $items;
}

/**
 * Implements hook_rules_condition_info().
 */
function entitysync_rules_condition_info() {
  $items = array();
  $items['entitysync_condition_invoked'] = array(
    'label' => t('Invoked by Entity Sync'),
    'group' => t('Entity Sync'),
  );
  return $items;
}


/**
 * Implements hook_form_alter().
 */
function _entitysync_action_form_alter(&$form, &$form_state, $options, RulesAbstractPlugin $element) {
  // Ripped from Rules.
  $first_step = empty($element->settings['entity:select']);
  $form['reload'] = array(
    '#weight' => 5,
    '#type' => 'submit',
    '#name' => 'reload',
    '#value' => $first_step ? t('Continue') : t('Reload form'),
    '#limit_validation_errors' => array(array('parameter', 'type')),
    '#submit' => array('rules_action_type_form_submit_rebuild'),
    '#ajax' => rules_ui_form_default_ajax(),
  );
  // Use ajax and trigger as reload button.
  $form['parameter']['entity']['settings']['type']['#ajax'] = $form['reload']['#ajax'] + array(
    'event' => 'change',
    'trigger_as' => array('name' => 'reload'),
  );

  if ($first_step) {
    // In the first step show only the type select.
    foreach (element_children($form['parameter']) as $key) {
      if ($key != 'entity') {
        unset($form['parameter'][$key]);
      }
    }
    unset($form['submit']);
    unset($form['provides']);
    // Disable #ajax for the first step as it has troubles with lazy-loaded JS.
    // @todo: Re-enable once JS lazy-loading is fixed in core.
    unset($form['parameter']['entity']['settings']['type']['#ajax']);
    unset($form['reload']['#ajax']);
  }
  else {
    // Hide the reload button in case js is enabled and it's not the first step.
    $form['reload']['#attributes'] = array('class' => array('rules-hide-js'));
  }
}

/**
 * Get available entity fields.
 */
function _entitysync_actions_get_entity_fields($element, $name = NULL) {
  $selector = $element->settings['entity:select'];
  $entity = $element->applyDataSelector($selector);
  $fields = array('-all-' => t('All aspects'));

  if ($entity) {
    $params = $element->pluginParameterInfo();
    $result = RulesData::matchingDataSelector($entity, $params['entity'], $selector . ':', 0);
    foreach ($result as $selector => $info) {
      if ($selector[strlen($selector) - 1] !== ':') {
        $fields[$info['name']] = $info['name'] . ' - ' . $info['label'];
      }
    }
  }
  return $fields;
}

/**
 * Condition for checking if this has been invoked by Entity Sync module.
 */
function entitysync_condition_invoked() {
  return arg(0) == 'entitysync';
}

/**
 * Rules sync access callback.
 */
function entitysync_access_callback() {
  return user_access("administer entitysync");
}

/**
 * Action: entity sync
 */
function entitysync_rules_sync($entity, $unique, $fields, $create, $endpoint) {

  require_once drupal_get_path('module', 'entitysync') . '/entitysync.inc';

  $entity_sync_packet = new EntitySyncPacket($entity, $unique, $fields);
  $entity_sync_packet->op = "sync";
  $entity_sync_packet->create = $create;

  // Invoke pre sync filter functions.
  drupal_alter('entitysync_send', $entity_sync_packet);

  _entitysync_relay_packet($endpoint, $entity_sync_packet);
}

/**
 * Action: entity deletion
 */
function entitysync_rules_del($entity, $unique, $endpoint) {

  require_once drupal_get_path('module', 'entitysync') . '/entitysync.inc';

  $entity_sync_packet = new EntitySyncPacket($entity, $unique);
  $entity_sync_packet->op = "del";

  // Invoke pre sync filter functions.
  drupal_alter('entitysync_send', $entity_sync_packet);

  _entitysync_relay_packet($endpoint, $entity_sync_packet);
}
