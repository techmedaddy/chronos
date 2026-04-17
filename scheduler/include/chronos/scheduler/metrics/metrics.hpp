#pragma once

#include <atomic>
#include <chrono>
#include <string>

namespace chronos::scheduler::metrics {

struct SchedulerMetrics {
  std::atomic<unsigned long long> election_attempts{0};
  std::atomic<unsigned long long> election_wins{0};
  std::atomic<unsigned long long> election_losses{0};
  std::atomic<unsigned long long> election_churn{0};
  std::atomic<unsigned long long> scan_batches{0};
  std::atomic<unsigned long long> dispatch_attempts{0};
  std::atomic<unsigned long long> dispatch_success{0};

  // Phase 8 metrics
  std::atomic<unsigned long long> retries_total{0};
  std::atomic<unsigned long long> dead_letters_total{0};
  std::atomic<unsigned long long> worker_success_total{0};
  std::atomic<unsigned long long> worker_failure_total{0};

  std::atomic<long long> queue_depth{0};
  std::atomic<long long> db_latency_ms{0};
  std::atomic<long long> scan_lag_ms{0};
  std::atomic<long long> dispatch_latency_ms{0};
  std::atomic<long long> task_latency_ms{0};
  std::atomic<long long> worker_utilization_pct{0};
  std::atomic<long long> active_leader{0};
  std::atomic<long long> execution_timeout_growth{0};
};

std::string ToPrometheus(const SchedulerMetrics& metrics);

}  // namespace chronos::scheduler::metrics
