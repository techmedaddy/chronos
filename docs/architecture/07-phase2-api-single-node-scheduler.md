# Phase 2: API Server + Single-Node Scheduler (Local Executor Mode)

This document records the implementation delivered for Phase 2.

## Implemented scope

### 2.1 REST endpoints

Implemented route handlers for:

- `POST /jobs`
- `POST /schedules`
- `GET /jobs/:id`
- `GET /jobs/:id/executions`
- `GET /health`
- `GET /metrics`

Files:

- `api-server/include/chronos/api/http/router.hpp`
- `api-server/src/http/router.cpp`
- `api-server/include/chronos/api/http/server.hpp`
- `api-server/src/http/server.cpp`
- `api-server/src/handlers/*.cpp`

### 2.2 Validation, auth middleware, request-id, structured logging

Implemented:

- Token auth middleware (`Authorization: Bearer <token>`) for all non-health endpoints
- Request ID propagation via `x-request-id` (or generated fallback)
- Structured JSON logging with service, level, request_id, message
- Payload validation stubs for create job / create schedule

Files:

- `api-server/src/middleware/auth_middleware.cpp`
- `api-server/src/middleware/request_id_middleware.cpp`
- `api-server/src/validation/job_validation.cpp`
- `api-server/src/observability/logger.cpp`

### 2.3 Single-node scheduler loop (one-time + cron)

Implemented scheduler tick logic:

- pull due schedules
- compute schedule decision
- apply misfire policy
- persist execution creation
- persist dispatch intent (outbox)
- transition to DISPATCHED
- handoff to local executor

Files:

- `scheduler/src/core/scheduler_loop.cpp`
- `scheduler/src/scheduling/schedule_calculator.cpp`
- `scheduler/src/scheduling/misfire_policy.cpp`
- `scheduler/src/scheduling/duplicate_guard.cpp`

### 2.4 Deterministic schedule calc + guardrails

Implemented:

- deterministic due decision based on persisted `next_run_at`
- simple cron next-run parser for `M H * * *` shape
- misfire detection with drift guard
- misfire action: `FIRE_ONCE` default, `SKIP` support
- duplicate-dispatch guard using in-process dispatch key set

### 2.5 Persist dispatch intent before publish

Implemented outbox write before transition and local execution pickup:

- `outbox_events` insertion with event type `execution.dispatch_intent`

### 2.6 Local integration flow without RabbitMQ

Implemented local executor path:

- execution state: `DISPATCHED -> RUNNING -> SUCCEEDED`
- no broker dependency required for Phase 2 local run

Files:

- `scheduler/src/executor/local_executor.cpp`

## Additional enabling work

To enable local Phase 2 execution without DB wiring delays:

- Added in-memory repository implementations for job/schedule/execution/audit/outbox.

Files:

- `shared-core/include/chronos/persistence/in_memory/in_memory_repositories.hpp`
- `shared-core/src/persistence/in_memory/in_memory_repositories.cpp`

## Build wiring

- Updated `api-server/CMakeLists.txt` with library + executable targets.
- Updated `scheduler/CMakeLists.txt` with library + executable targets.
- Updated `shared-core/CMakeLists.txt` to include in-memory repository source.

## Validation status

- Source-level syntax checks pass across `shared-core`, `api-server`, and `scheduler` (`g++ -fsyntax-only`).
- Full CMake configure/build is blocked on host because `cmake` is missing in PATH.

## Exit criteria status

- API endpoints implemented: **done**
- Validation/auth/request-id/logging: **done (MVP level)**
- Single-node scheduler loop: **done**
- Deterministic next-run/misfire/duplicate guards: **done (MVP level)**
- Persist dispatch intent before pickup: **done**
- Local non-RabbitMQ integration flow: **done**

## Known MVP limitations (expected)

1. HTTP server is in-process router interface (not yet bound to real TCP listener/framework).
2. Validation currently uses lightweight string-token checks, not full JSON schema validation.
3. Cron parser currently supports a simple `M H * * *` subset.
4. Duplicate dispatch guard is process-local; distributed dedupe should move to durable lock/idempotency in later phases.
5. PostgreSQL repository adapter still contains TODO SQL wiring from Phase 1.
