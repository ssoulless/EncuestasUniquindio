<?php
/**
 * @file
 * Entity Sync Queue class
 */

class EntitySyncQueue {

  private $cron_limit;

  /**
   * Constructor
   */
  public function __construct() {
    $this->cron_limit = 25;
  }

  /**
   * Process queue (on cron runs)
   */
  public function processQueue() {

    $queued_packets = db_select('entitysync_packet_queue', 'epq')
      ->fields('epq', array(
        'id',
        'endpoint',
        'packet',
      ))
      ->orderBy('epq.id')
      ->range(0, $this->cron_limit)
      ->execute();

    $skip_endpoint = array();
    $delete = array();

    while ($queued_packet = $queued_packets->fetchObject()) {
      $packet = new EntitySyncPacket();
      $packet->decode($queued_packet->packet);
      // Relay packet but do not queue
      if (_entitysync_relay_packet($queued_packet->endpoint, $packet, FALSE)) {
        $delete[] = $queued_packet->id;
      }
    }

    if (count($delete) > 0) {
      db_delete('entitysync_packet_queue')
        ->condition('id', $delete, 'IN')
        ->execute();
    }

    // Update error count
    db_update('entitysync_packet_queue')
      ->expression('errors', 'errors + 1')
      ->execute();

    // Delete errors that have gone over the limit
    db_delete('entitysync_packet_queue')
      ->condition('errors', variable_get('entitysync_error_limit', 5), '>')
      ->execute();
  }

  /**
   * Add the packet to queue
   * @param $endpoint
   *   Host URL
   * @param $packet EntitySyncPacket
   *   Packet to queue
   */
  public function queuePacket($endpoint, EntitySyncPacket &$packet) {

    // Queueing disabled.
    if (variable_get('entitysync_error_limit', 5) == 0) {
      return;
    }

    // Add obj to queue.
    db_insert('entitysync_packet_queue')
      ->fields(array(
        'endpoint' => $endpoint,
        'queue_identifier' => $packet->getQueueIdentifier(),
        'packet' => $packet->encoded(),
      ))
      ->execute();
  }

  /**
   * Test if endpoint has queued packets
   * @param $endpoint
   *   Host URL
   * @return boolean
   *   FALSE if not TRUE if endpoint has queued packets
   */
  public function endpointHasQueuedPackets($endpoint) {
    $endpoints = db_select('entitysync_packet_queue', 'epq')
      ->fields('epq', array('endpoint'))
      ->condition('endpoint', $endpoint)
      ->range(0, 1)
      ->execute();
    return $endpoints->rowCount() > 0;
  }

}
