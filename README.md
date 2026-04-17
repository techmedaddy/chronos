# Chronos

Chronos is a distributed job scheduling and execution system in C++.

This repository starts with the system architecture and module boundaries so
implementation can proceed without service responsibilities drifting over time.

## Concrete defaults

- Language standard: `C++20`
- Build system: `CMake`
- Source of truth database: `PostgreSQL`
- Message broker: `RabbitMQ`
- Advisory cache and locks: `Redis`
- Scheduler leader election: `etcd`
- Local development platform: `Docker Compose`
- Future deployment target: `Kubernetes`

## Local full backend (single compose file)

Per your deployment preference, complete backend development stack runs from:

- `docker-compose.yml` (repo root)

Start stack:

```bash
docker compose up -d --build
```

Scale workers locally:

```bash
docker compose up -d --scale worker=10
```

## Initial module layout

- `api-server/` accepts job and schedule requests, validates them, persists them,
  and exposes query APIs.
- `scheduler/` computes due work and dispatches executions.
- `worker/` consumes execution messages and runs task handlers.
- `shared-core/` contains domain models, shared contracts, config, utilities, and
  infrastructure adapters that are safe to reuse across services.
- `db-migrations/` owns schema versioning and database bootstrap artifacts.
- `ops/` contains local dev, deployment, and observability assets.
- `docs/` stores architecture decisions and design documents.
- `k8s/` contains baseline Kubernetes manifests and policies.

## Architecture docs

- `docs/architecture/01-system-boundaries.md`
- `docs/architecture/02-concrete-defaults.md`
- `docs/architecture/03-domain-model.md`
- `docs/architecture/04-interface-contracts.md`
- `docs/architecture/05-developer-baseline.md`
- `docs/architecture/06-phase1-persistence-mvp.md`
- `docs/architecture/07-phase2-api-single-node-scheduler.md`
- `docs/architecture/08-phase3-worker-runtime.md`
- `docs/architecture/09-phase4-rabbitmq-reliable-dispatch.md`
- `docs/architecture/10-phase5-retry-recovery-failure-management.md`
- `docs/architecture/11-phase6-distributed-scheduler-leader-election.md`
- `docs/architecture/12-phase7-redis-coordination-and-phase8-observability-foundation.md`
- `docs/architecture/13-phase8-observability-operability.md`
- `docs/architecture/14-phase9-containerization-orchestration-scale.md`
