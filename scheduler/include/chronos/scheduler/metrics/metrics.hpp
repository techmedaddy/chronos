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

  std::atomic<long long> scan_lag_ms{0};
  std::atomic<long long> dispatch_latency_ms{0};
};

std::string ToPrometheus(const SchedulerMetrics& metrics);

}  // namespace chronos::scheduler::metrics
