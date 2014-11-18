
Entity Sync
---------------------

Synchronize entities or their aspects (fields or non field data) between
Drupal installations with Rules (http://drupal.org/project/rules).

Instructions
---------------------

Install and enable the module on both Drupal installations. Create a rule which
triggers the Synchronize entity -action. To prevent loops filter the event
with negated Invoked by Entity Sync -condition. In the action configuration:

 1. Select an entity which will be synchronized.
 2. Select a unique aspect which is used to identify the entity on both ends.
 3. Select aspects which will be synchronized.
 4. Add the endpoint URLs. The Entity Sync module responds in
    http://www.example.com/entitysync

Repeat for the other Drupal installation. Note that you can also export the
Rule and just swap the endpoint URL :)

Packet queues
---------------------

Entity Sync has an internal queue for failed packets which means that when
an endpoint becomes unresponsive or begins failing all outgoing packets are
queued. Queue is processed on cron runs and it will be opened when the queue
for the given endpoint has been cleared.

Hooks
---------------------

Should you need to alter the packets or piggyback custom data, these two hooks
are provided:

Outgoing packets:
  function hook_entitysync_send_alter(EntitySyncPacket $packet)

Receiving packets:
  function hook_entitysync_receive_alter(EntitySyncPacket $packet)
