# worker

Workers consume execution messages and run task handlers asynchronously.

They own execution runtime concerns such as claiming, heartbeats, timeouts,
retries, and graceful shutdown.

## Phase 4 status

Implemented RabbitMQ-style consumer flow (MVP) with:

- consume from `main_queue` and `retry_queue`
- manual `ack` after successful runtime submission
- `nack` + reroute to `retry_queue` on runtime rejection
- `nack` + reroute to `dead_letter_queue` on decode failure

DB remains source of truth because claim/state transition occurs before message ack.

Current implementation runs against queue broker abstraction with in-memory broker in local mode.
