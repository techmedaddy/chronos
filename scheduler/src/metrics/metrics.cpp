#include "chronos/scheduler/metrics/metrics.hpp"

namespace chronos::scheduler::metrics {

std::string ToPrometheus(const SchedulerMetrics& metrics) {
  std::string out;
  out += "chronos_scheduler_election_attempts_total " + std::to_string(metrics.election_attempts.load()) + "\n";
  out += "chronos_scheduler_election_wins_total " + std::to_string(metrics.election_wins.load()) + "\n";
  out += "chronos_scheduler_election_losses_total " + std::to_string(metrics.election_losses.load()) + "\n";
  out += "chronos_scheduler_election_churn_total " + std::to_string(metrics.election_churn.load()) + "\n";
  out += "chronos_scheduler_scan_batches_total " + std::to_string(metrics.scan_batches.load()) + "\n";
  out += "chronos_scheduler_dispatch_attempts_total " + std::to_string(metrics.dispatch_attempts.load()) + "\n";
  out += "chronos_scheduler_dispatch_success_total " + std::to_string(metrics.dispatch_success.load()) + "\n";
  out += "chronos_scheduler_retries_total " + std::to_string(metrics.retries_total.load()) + "\n";
  out += "chronos_scheduler_dead_letters_total " + std::to_string(metrics.dead_letters_total.load()) + "\n";
  out += "chronos_scheduler_worker_success_total " + std::to_string(metrics.worker_success_total.load()) + "\n";
  out += "chronos_scheduler_worker_failure_total " + std::to_string(metrics.worker_failure_total.load()) + "\n";
  out += "chronos_scheduler_queue_depth " + std::to_string(metrics.queue_depth.load()) + "\n";
  out += "chronos_scheduler_db_latency_ms " + std::to_string(metrics.db_latency_ms.load()) + "\n";
  out += "chronos_scheduler_scan_lag_ms " + std::to_string(metrics.scan_lag_ms.load()) + "\n";
  out += "chronos_scheduler_dispatch_latency_ms " + std::to_string(metrics.dispatch_latency_ms.load()) + "\n";
  out += "chronos_scheduler_task_latency_ms " + std::to_string(metrics.task_latency_ms.load()) + "\n";
  out += "chronos_scheduler_worker_utilization_pct " + std::to_string(metrics.worker_utilization_pct.load()) + "\n";
  out += "chronos_scheduler_active_leader " + std::to_string(metrics.active_leader.load()) + "\n";
  out += "chronos_scheduler_execution_timeout_growth " + std::to_string(metrics.execution_timeout_growth.load()) + "\n";
  return out;
}

}  // namespace chronos::scheduler::metrics
