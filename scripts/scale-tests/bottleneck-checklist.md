# Scale Test Bottleneck Checklist

## 1) DB write throughput

- Check `chronos_db_latency_ms` trend under worker scale-up
- Watch PostgreSQL connection saturation
- Validate insert/update contention on executions/attempts tables

## 2) Queue saturation

- Check `chronos_queue_depth`
- Compare dispatch rate vs consume/ack rate
- Watch RabbitMQ memory alarm and channel/blocking behavior

## 3) Scheduler scan cost

- Check `chronos_scheduler_scan_lag_ms`
- Check batched catch-up behavior under large due backlog
- Verify lease churn is not causing scan stalls

## 4) Worker-side pressure

- Check `chronos_worker_utilization_pct`
- Observe task latency p95/p99 and timeout growth
- Validate no uncontrolled retry storms

## 5) Stability checks

- Rolling restart API/scheduler/worker and confirm no job stranding
- Verify leader handoff does not duplicate beyond at-least-once contract
- Confirm dead-letter growth remains bounded and explainable
