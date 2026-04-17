# Phase 4: RabbitMQ Integration and Reliable Dispatch (MVP)

This document records Phase 4 implementation status.

## Implemented scope

### 4.1 Queues introduced

Defined canonical queue names:

- `main_queue`
- `retry_queue`
- `dead_letter_queue`

File:

- `shared-core/include/chronos/messaging/message_codec.hpp`

### 4.2 Message schema

Defined dispatch message schema fields:

- `job_id`
- `execution_id`
- `attempt`
- `scheduled_at`
- `idempotency_key`
- `trace_id`

plus `message_type` and `message_version`.

Files:

- `shared-core/include/chronos/messaging/message_codec.hpp`
- `shared-core/src/messaging/message_codec.cpp`

### 4.3 Scheduler publisher path with confirms

Implemented scheduler-side RabbitMQ publisher abstraction:

- serializes dispatch message
- publishes to target queue
- enforces publisher confirm requirement when requested

Files:

- `scheduler/include/chronos/scheduler/messaging/rabbitmq_publisher.hpp`
- `scheduler/src/messaging/rabbitmq_publisher.cpp`

### 4.4 Consumer path with manual ack/nack + retry routing

Implemented worker-side consumer abstraction:

- consumes from `main_queue` / `retry_queue`
- decodes message
- on decode error -> `nack` to `dead_letter_queue`
- on runtime rejection/claim failure -> `nack` to `retry_queue`
- on successful runtime submission -> `ack`

Files:

- `worker/include/chronos/worker/messaging/rabbitmq_consumer.hpp`
- `worker/src/messaging/rabbitmq_consumer.cpp`

### 4.5 DB as source of truth before ack

Enforced by flow design:

- worker runtime claim uses repository transition (`DISPATCHED -> RUNNING`) before consumer ACK.
- if claim/submit fails, message is not ACKed and is routed for retry.

Related files:

- `worker/src/runtime/worker_runtime.cpp`
- `worker/src/execution/claim_manager.cpp`
- `worker/src/messaging/rabbitmq_consumer.cpp`

### 4.6 Reconciliation job (DB/queue drift repair)

Implemented reconciliation job that:

- detects drift condition (expected inflight executions + empty queue)
- re-publishes missing dispatches from DB execution state

Files:

- `scheduler/include/chronos/scheduler/reconciliation/reconciliation_job.hpp`
- `scheduler/src/reconciliation/reconciliation_job.cpp`

## Infrastructure abstraction used for local MVP

Added queue broker interface and in-memory broker implementation with:

- publish
- consume
- manual ack
- nack with requeue/reroute
- queue depth inspection

Files:

- `shared-core/include/chronos/messaging/queue_broker.hpp`
- `shared-core/include/chronos/messaging/in_memory_queue_broker.hpp`
- `shared-core/src/messaging/in_memory_queue_broker.cpp`

This enables local reliable-dispatch behavior without requiring RabbitMQ daemon during early iterations.

## Scheduler dispatch flow update

Scheduler now:

1. persists execution in DB (`PENDING`)
2. persists outbox dispatch intent
3. transitions to `DISPATCHED`
4. publishes queue dispatch message with confirm
5. leaves repairability to reconciliation when publish path fails

File:

- `scheduler/src/core/scheduler_loop.cpp`

## Validation status

- All translation units in `shared-core`, `api-server`, `scheduler`, and `worker` pass `g++ -fsyntax-only` checks.
- Full CMake build remains blocked until `cmake` is available in PATH.

## Exit criteria status (Phase 4 MVP)

- main/retry/dead-letter queues: **done**
- message schema fields: **done**
- scheduler publisher confirms: **done (abstraction-level)**
- worker manual ack/nack + retry routing: **done**
- DB-before-ack flow: **done**
- drift reconciliation job: **done**

## Known MVP limitations

1. RabbitMQ is represented by a queue broker abstraction + in-memory broker in this phase; production AMQP client wiring is next.
2. Dispatch message decode is lightweight and should be replaced with robust JSON parser.
3. Reconciliation currently receives expected execution IDs from caller; production version should query authoritative DB windows directly.
4. Outbox delivery daemon is not implemented yet (outbox write exists; dispatcher loop pending).
