<?php
/**
 * @file
 * Entity Sync callback related functions.
 */

/**
 * Entity update callback.
 */
function entitysync_sync_callback() {

  require_once drupal_get_path('module', 'entitysync') . '/entitysync.inc';

  $entity_sync_packet = new EntitySyncPacket();
  // Process and validate data.
  if (!$entity_sync_packet->decode(file_get_contents('php://input'))) {
    drupal_add_http_header('Status', '403 Forbidden');
    drupal_exit();
  }

  drupal_alter('entitysync_receive', $entity_sync_packet);

  // Fetch the entity.
  $entity = $entity_sync_packet->fetchEntity();

  // No entity -> fail.
  if (!$entity) {
    drupal_add_http_header('Status', '409 Conflict');
    drupal_exit();
  }

  $field_handlers = entitysync_get_field_handlers();

  switch ($entity_sync_packet->op) {

    case 'del':
      // Delete, but only try to delete existing entities.
      if (!isset($entity->is_new)) {
        $info = entity_get_info($entity_sync_packet->entityType);
        $id = $info['entity keys']['id'];
        entity_delete($entity_sync_packet->entityType, $entity->$id);
      }
      break;

    case 'sync':
      if (!$entity_sync_packet->create && $entity->is_new) {
        // Skip since we do not want to create an entity
        break;
      }
      // Update entity fields.
      foreach ($entity_sync_packet->fields as $field_name => $field_data) {
        $info = field_info_field($field_name);
        if (isset($field_handlers[$info['type']])) {
          $entity->$field_name = $field_handlers[$info['type']]['in']($entity, $field_name, $field_data);
        } else {
          $entity->$field_name = $field_data;
        }
      }
      entity_save($entity_sync_packet->entityType, $entity);
      break;

  }

  drupal_exit();
}
