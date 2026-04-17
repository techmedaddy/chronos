# Phase 8: Observability and Operability (MVP)

This document records implementation delivered for Phase 8.

## 8.1 Prometheus metrics

Added/extended metrics coverage for:

- schedule lag
- queue depth
- dispatch rate
- success/failure counts
- retry counts
- worker utilization
- task latency
- DB latency

Files:

- `api-server/include/chronos/api/handlers/context.hpp`
- `api-server/src/handlers/metrics_handlers.cpp`
- `scheduler/include/chronos/scheduler/metrics/metrics.hpp`
- `scheduler/src/metrics/metrics.cpp`
- `scheduler/src/core/scheduler_runtime.cpp`

## 8.2 Structured logs with required fields

Updated logging guidance and key lifecycle logs to include:

- `job_id`
- `execution_id`
- `attempt`
- `worker_id`
- `trace_id`

Files:

- `worker/src/observability/logger.cpp`
- `worker/src/runtime/worker_runtime.cpp`
- `scheduler/src/core/scheduler_loop.cpp`

## 8.3 Elastic Stack shipping and searchable dashboards

Added Filebeat + Elastic/Kibana baseline config:

- filebeat JSON decode and Elasticsearch output
- compose stack entries for Elasticsearch, Kibana, Filebeat

Files:

- `ops/observability/elastic/filebeat/filebeat.yml`
- `ops/observability/compose-observability.yml`

## 8.4 Grafana dashboards

Added initial Grafana dashboard JSON including panels for:

- scheduler health
- retries/dead letters trends
- worker utilization
- queue depth
- task and DB latency

File:

- `ops/observability/grafana/dashboards/chronos-overview.json`

## 8.5 Alerts

Added Prometheus alert rules for:

- queue backlog spike
- rising failure rate
- no active scheduler leader
- execution timeout growth

File:

- `ops/observability/alerts/chronos-alerts.yml`

## Operability runbook

Added on-call playbook for:

- metric interpretation
- log search workflow
- dashboard usage
- alert response sequences

File:

- `ops/runbooks/observability-operability.md`

## Validation status

- syntax checks pass for all C++ modules (`g++ -fsyntax-only`).
- observability configs are file-complete and ready for local compose bring-up.

## Known MVP limitations

1. Metric values for some gauges are currently derived placeholders in local runtime; production collectors should wire real broker/DB stats.
2. Elastic templates/ILM policies are not yet added.
3. Grafana provisioning manifests are not yet included (dashboard JSON provided).
4. Alertmanager routing config is not yet included.
