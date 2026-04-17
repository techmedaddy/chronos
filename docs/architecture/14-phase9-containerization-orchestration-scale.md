# Phase 9: Containerization, Orchestration, and Scale (MVP)

This document records implementation delivered for Phase 9.

## User constraint honored

You requested **one root `docker-compose.yml` file for complete backend**.
This has been implemented.

## 9.1 Docker images for each service

Added service-specific Dockerfiles:

- `docker/Dockerfile.api-server`
- `docker/Dockerfile.scheduler`
- `docker/Dockerfile.worker`

All use multi-stage build and produce runnable service images.

## 9.2 One root docker-compose stack

Added root stack file:

- `docker-compose.yml`

Includes full backend + observability dependencies:

- api-server
- scheduler
- worker (scalable replicas)
- PostgreSQL
- RabbitMQ
- Redis
- etcd
- Prometheus
- Grafana
- Elasticsearch
- Kibana
- Filebeat

## 9.3 Kubernetes manifests

Added baseline manifests under `k8s/` for:

- API
- scheduler
- worker
- RabbitMQ
- Redis
- PostgreSQL
- etcd

Files include namespace, deployments/statefulset, and service where needed.

## 9.4 Worker autoscaling policy

Added HPA:

- `k8s/autoscaling/worker-hpa.yaml`

Policy scales worker based on:

- CPU utilization
- queue depth metric (`chronos_queue_depth`)

## 9.5 Probes, disruption budgets, rolling rules

Added:

- readiness/liveness probes in deployments
- rolling update strategy fields
- PDBs:
  - `k8s/policies/pdb-api.yaml`
  - `k8s/policies/pdb-scheduler.yaml`
  - `k8s/policies/pdb-worker.yaml`

## 9.6 Scale tests + bottleneck identification assets

Added scripts/docs:

- `scripts/scale-tests/run-compose-scale.sh`
- `scripts/scale-tests/load-generator.sh`
- `scripts/scale-tests/bottleneck-checklist.md`

Targets bottlenecks requested:

- DB write throughput
- queue saturation
- scheduler scan cost

## Exit criteria status (Phase 9 MVP)

- Horizontal scaling by adding workers: **done** (compose scale + HPA policy)
- Deployments/restarts routine and safer: **done** (probes + PDB + rolling strategy)
- Single root compose backend file: **done**

## Known MVP limitations

1. Health probes currently placeholder exec checks; wire HTTP/metrics-based probes when service listeners are finalized.
2. K8s manifests are baseline YAML (not Helm chart yet).
3. HPA queue-depth metric requires metrics pipeline adapter/custom metrics API in cluster.
4. Compose `deploy.replicas` is effective in swarm mode; use `docker compose up --scale worker=N` for local compose.
