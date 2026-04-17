# Kubernetes manifests

This folder contains baseline manifests for Chronos services and infrastructure.

## Layout

- `base/` namespace
- `api/` API deployment + service
- `scheduler/` scheduler deployment
- `worker/` worker deployment
- `infrastructure/` RabbitMQ, Redis, PostgreSQL, etcd
- `autoscaling/` worker HPA policy
- `policies/` PDBs

## Notes

- Readiness/liveness probes, rolling updates, and PDBs are included.
- Worker autoscaling policy uses CPU + queue-depth metric target.
- For production, replace placeholder probes with real health endpoints.
