# scheduler

The scheduler is the orchestration component that detects due work and dispatches
execution attempts.

It is the only component that decides when a job should run.

## Phase 2 status

Implemented single-node scheduler tick pipeline:

1. reads due schedules
2. computes deterministic due/misfire/next-run decisions
3. applies misfire policy
4. prevents duplicate dispatches via dispatch-key guard
5. persists execution + dispatch intent (outbox)
6. transitions execution to `DISPATCHED`
7. invokes local executor (no RabbitMQ path) for local integration mode

Also includes scheduling helpers for:

- simple cron next-run calculation (`M H * * *` subset)
- drift guardrails
- misfire actions (`FIRE_ONCE`, `SKIP`)
