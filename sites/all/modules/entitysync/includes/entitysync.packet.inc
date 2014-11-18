<?php
/**
 * @file
 * Definition of EntitySyncPacket class.
 */

class EntitySyncPacket {

  public $uniqueField;
  public $uniqueValue;
  public $entityType;
  public $bundle;
  public $fields;
  public $checksum;
  public $op;
  public $create;

  /**
   * When both parameters are given, a packet is created from the given entity
   * using the unique field as a key.
   *
   * @param $entity
   *   object|NULL DrupalEntityWrapper object
   * @param $unique
   *   object|NULL
   * @param $fields
   *   array Array of fields to sync
   */
  public function __construct($entity = NULL, $unique = NULL, $fields = array()) {

    if ($entity == NULL) {
      return;
    }

    $info        = $unique->info();
    $field_name  = $info['name'];
    $entity_type = $entity->type();
    $bundle      = $entity->getBundle();

    // Prepare syncing.
    $this->uniqueField = $field_name;
    $this->uniqueValue = $unique->value();
    $this->entityType  = $entity_type;
    $this->bundle      = $bundle;
    $this->fields      = array();

    // Collect filtered fields.
    $filter_fields = array();
    $entity_info = $entity->entityInfo();

    foreach ($entity_info['entity keys'] as $id => $key) {
      if ($id != 'label') {
        $filter_fields[] = $key;
      }
    }

    // Collect the whole entity.
    if (isset($fields['-all-'])) {
      $fields = $entity->getPropertyInfo();
      $fields = array_keys($fields);
    }

    $entity_raw = $entity->raw();

    $field_handlers = entitysync_get_field_handlers();

    foreach ($fields as $field_name) {
      if (!in_array($field_name, $filter_fields) &&
          isset($entity_raw->$field_name)) {
        $info = field_info_field($field_name);
        if (isset($field_handlers[$info['type']])) {
          $this->fields[$field_name] = $field_handlers[$info['type']]['out']($entity->$field_name, $field_name, $entity_raw->$field_name);
        } else {
          // Other fields.
          $this->fields[$field_name] = $entity_raw->$field_name;
        }
      }
    }
  }

  /**
   * Decode a json object to this packet.
   *
   * @return boolean
   *   TRUE or FALSE signifying if the packet is valid (trustworthy) or not
   */
  public function decode($json) {
    $obj = drupal_json_decode($json);
    foreach ($obj as $key => $value) {
      $this->$key = $value;
    }
    return $this->validate();
  }

  /**
   * Get encoded version of this packet.
   *
   * @return string
   *   Encoded packet
   */
  public function encoded() {
    $this->checksum = $this->generateChecksum();
    return drupal_json_encode($this);
  }

  /**
   * Generate checksum for this packet
   * @return string
   *    An MD5 checksum for this packet
   */
  private function generateChecksum() {
    return md5(variable_get('entitysync_shared_secret', 'undefined') . serialize($this->fields) . $this->getQueueIdentifier());
  }

  /**
   * Fetch a Drupal entity for this packet.
   *
   * @return object|boolean
   *   Drupal entity object or FALSE in case of an error
   */
  public function fetchEntity() {
    $entity_field_query = new EntityFieldQuery();
    $entity_field_query->entityCondition('entity_type', $this->entityType)
      ->propertyCondition($this->uniqueField, $this->uniqueValue);
    // ->entityCondition('bundle', $this->bundle)

    $entities = $entity_field_query->execute();
    $ids = array_keys($entities[$this->entityType]);
    $number_of_entities = count($ids);

    if ($number_of_entities > 0) {
      $entities = entity_load($this->entityType, $ids);
      $number_of_entities = count($entities);
    }

    if ($number_of_entities == 0) {
      // Create a new one.
      $info = entity_get_info($this->entityType);
      $bundle = $info['entity keys']['bundle'];
      $new = (object)NULL;
      $new->is_new = TRUE;
      if (!empty($bundle)) {
        $new->$bundle = $this->bundle;
      }
      return $new;
    }
    elseif ($number_of_entities == 1) {
      // Update existing.
      return array_shift($entities);
    }
    // Generate error.
    return FALSE;
  }

  /**
   * Validate the packet.
   *
   * @return boolean
   *   TRUE if the packet is OK, FALSE if not
   */
  protected function validate() {
    return $this->checksum == $this->generateChecksum();
  }

  /**
   * Get the queue identifier for this packet
   *
   * @return string
   *   String which identifies the entity
   */
  public function getQueueIdentifier() {
    return $this->entityType . ':'
      . $this->bundle . ':'
      . $this->uniqueField . ':'
      . $this->uniqueValue;
  }

};
