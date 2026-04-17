# Chronos Observability & Operability Runbook

## Metrics to monitor

- `chronos_schedule_lag_ms`
- `chronos_queue_depth`
- `chronos_dispatch_rate_total`
- `chronos_success_total`
- `chronos_failure_total`
- `chronos_retry_total`
- `chronos_worker_utilization_pct`
- `chronos_task_latency_ms`
- `chronos_db_latency_ms`
- `chronos_scheduler_active_leader`
- `chronos_scheduler_execution_timeout_growth`

## Structured log contract

Every lifecycle log should include:

- `job_id`
- `execution_id`
- `attempt`
- `worker_id`
- `trace_id`

## Elastic/Kibana

1. Start stack from `ops/observability/compose-observability.yml`.
2. Ensure Filebeat has access to `/var/log/chronos/*.log`.
3. In Kibana, create data view for index pattern `chronos-logs-*`.
4. Create saved searches for:
   - failed dispatches
   - retry storms
   - timeout-heavy executions

## Grafana dashboards

Import `ops/observability/grafana/dashboards/chronos-overview.json`.

Recommended dashboard folders:

- Scheduler Health
- Worker Health
- Retries & Dead Letters
- DB Load & Latency

## Alert response playbook

### Queue backlog spike

- verify scheduler leadership (`chronos_scheduler_active_leader`)
- inspect worker utilization and queue depth trend
- scale worker replicas if sustained > 10 minutes

### Rising failure rate

- inspect logs by `trace_id` and `execution_id`
- identify dominant `last_error_code`
- verify downstream dependency health

### No active scheduler leader

- inspect lease churn metrics
- check etcd connectivity/latency
- force one scheduler restart if election deadlock suspected

### Execution timeout growth

- inspect task latency p95/p99
- inspect DB latency and queue depth correlation
- increase timeout only after root-cause review
