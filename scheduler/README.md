# scheduler

The scheduler is the orchestration component that detects due work and dispatches
execution attempts.

It is the only component that decides when a job should run.

## Phase 4 status

Implemented reliable dispatch pipeline (MVP) with:

- dispatch message schema + versioning
- queue routing to `main_queue`
- publisher confirm semantics via broker abstraction
- persisted outbox dispatch intent before publish
- DB-first state transition to `DISPATCHED`
- reconciliation job for DB/queue drift repair

Current local mode uses in-memory broker implementing manual publish/consume/ack/nack semantics.
