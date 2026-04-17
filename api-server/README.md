# api-server

The API server is the control-plane entrypoint.

It owns request validation, authentication, persistence-facing command handling,
and query endpoints for job and execution state.

## Phase 2 status

Implemented in-process HTTP routing primitives and handlers for:

- `POST /jobs`
- `POST /schedules`
- `GET /jobs/:id`
- `GET /jobs/:id/executions`
- `GET /health`
- `GET /metrics`

Also includes:

- bearer-token auth middleware
- request-id propagation (`x-request-id`)
- structured logging helper
- payload validation stubs

Current executable (`chronos-api-server`) initializes server wiring in-process.
A real network HTTP listener can be attached in a follow-up phase.
