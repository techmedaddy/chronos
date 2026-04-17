# ops

Operations assets live here, including local development setup, container build
definitions, deployment manifests, observability bootstrap, and runbooks.

## Observability stack (Phase 8)

- Prometheus scrape + alert rules:
  - `ops/observability/prometheus/prometheus.yml`
  - `ops/observability/alerts/chronos-alerts.yml`
- Grafana dashboard JSON:
  - `ops/observability/grafana/dashboards/chronos-overview.json`
- Elastic ingestion via Filebeat:
  - `ops/observability/elastic/filebeat/filebeat.yml`
- Local compose for observability stack:
  - `ops/observability/compose-observability.yml`
- Runbook:
  - `ops/runbooks/observability-operability.md`
