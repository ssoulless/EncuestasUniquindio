<?php
/**
 * @file
 * Entity Sync includes
 */


/**
 * Get field handlers
 */
function entitysync_get_field_handlers() {
  static $handlers = NULL;

  if (!isset($handlers)) {

    $handlers = array();

    $file_handler = array(
      'in' => 'entitysync_file_in_handler',
      'out' => 'entitysync_file_out_handler',
    );

    $handlers['image'] = $file_handler;
    $handlers['file'] = $file_handler;

    drupal_alter('entitysync_field_handlers', $handlers);
  }
  return $handlers;
}

/**
 * Relay packet
 */
function _entitysync_relay_packet($endpoints, EntitySyncPacket &$packet, $process_queue = TRUE) {

  $options = array(
    'method' => 'POST',
    'data' => $packet->encoded(),
    'timeout' => 15,
    'headers' => array('Content-Type' => 'application/json'),
  );

  // Get endpoint target URLs.
  $endpoints = explode("\n", $endpoints);

  // Prepare variables for logging.
  $variables = array(
    '@entityType'  => $packet->entityType,
    '@bundle'      => $packet->bundle,
    '@uniqueField' => $packet->uniqueField,
    '@uniqueValue' => $packet->uniqueValue,
  );

  $queue = new EntitySyncQueue();

  $failed = FALSE;

  foreach ($endpoints as $endpoint) {

    $endpoint = trim($endpoint);

    // Queue packets for endpoint which have queued packets.
    if ($process_queue && $queue->endpointHasQueuedPackets($endpoint)) {
      $queue->queuePacket($endpoint, $packet);
      continue;
    }

    $variables['@endpoint'] = $endpoint;
    $result = drupal_http_request(trim($endpoint), $options);

    if (isset($result->error)) {
      $message = 'Syncing of @entityType.@bundle[@uniqueField]=@uniqueValue failed to @endpoint with code @code: @error';
      $variables['@error'] = $result->error;
      $variables['@code'] = $result->code;

      // drupal_set_message(t($message, $variables), 'error');
      watchdog('entitysync', $message, $variables, WATCHDOG_ERROR);

      // Queue packet for resending.
      if ($process_queue) {
        $queue->queuePacket($endpoint, $packet);
      }

      $failed = TRUE;
    }
    else {
      $message = 'Syncing of @entityType.@bundle[@uniqueField]=@uniqueValue to @endpoint succeeded.';
      // drupal_set_message(t($message, $variables), 'status');
      watchdog('entitysync', $message, $variables, WATCHDOG_NOTICE);
    }
  }
  return !$failed;
}


/**
 * Entity Sync file field inbound handler
 */
function entitysync_file_in_handler(&$entity, &$field_name, &$field_data) {

  $data = array();
  $ignore = array('uri', 'data');
  foreach ($field_data as $lang => &$files) {
    foreach ($files as $index => $file) {

      $destination_uri = $file['uri'];

      // Ensure that the target directory exists.
      $dir = drupal_dirname($destination_uri);
      drupal_mkdir($dir, NULL, TRUE);

      $fids = db_select('file_managed', 'fm')
          ->fields('fm', array('fid'))
          ->condition('uri', $destination_uri)
          ->execute();

      $fid = $fids->fetchObject();

      if ($fid->fid > 0) {
        // Existing file.
        $drupal_file = file_load($fid->fid);
      } else {
        // New file.
        $drupal_file = (object) NULL;
        $drupal_file->fid = NULL;
        $drupal_file->uid = 1;
        $drupal_file->status = 1;
      }

      foreach ($file as $key => $value) {
        if (!in_array($key, $ignore)) {
          $drupal_file->$key = $value;
        }
      }

      file_put_contents($destination_uri, base64_decode($file['data']));

      $drupal_file->uri = $destination_uri;
      $drupal_file->filename = drupal_basename($destination_uri);
      $drupal_file->filemime = file_get_mimetype($destination_uri);

      $drupal_file = file_save($drupal_file);

      $data[$lang][$index] = (array) $drupal_file;
    }
  }
  return $data;
}


/**
 * Entity Sync file field outbound handler
 */
function entitysync_file_out_handler(&$entity, &$field_name, &$field_data) {
  $data = array();
  $ignore = array('fid', 'vid', 'upload_button', 'remove_button', 'upload', 'weight', 'height');
  foreach ($field_data as $lang => &$files) {
    foreach ($files as $index => $file) {
      $drupal_file = file_load($file['fid']);
      $file_data = array();
      foreach ($file as $key => $value) {
        if (!in_array($key, $ignore)) {
          $file_data[$key] = $value;
        }
      }
      $file_data['uri'] = $drupal_file->uri;
      $file_data['data'] = base64_encode(file_get_contents($drupal_file->uri));
      $data[$lang][$index] = $file_data;
    }
  }
  return $data;
}
