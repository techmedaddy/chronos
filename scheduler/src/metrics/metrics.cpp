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
  out += "chronos_scheduler_scan_lag_ms " + std::to_string(metrics.scan_lag_ms.load()) + "\n";
  out += "chronos_scheduler_dispatch_latency_ms " + std::to_string(metrics.dispatch_latency_ms.load()) + "\n";
  return out;
}

}  // namespace chronos::scheduler::metrics
