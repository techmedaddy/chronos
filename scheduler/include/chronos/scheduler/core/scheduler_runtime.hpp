#pragma once

#include <chrono>
#include <memory>
#include <random>
#include <string>

#include "chronos/scheduler/core/scheduler_loop.hpp"
#include "chronos/scheduler/leader/lease_election.hpp"
#include "chronos/scheduler/leader/fencing_guard.hpp"
#include "chronos/scheduler/metrics/metrics.hpp"

namespace chronos::scheduler::core {

class SchedulerRuntime {
 public:
  struct Config {
    std::chrono::milliseconds base_loop_interval{1000};
    std::chrono::milliseconds max_jitter{250};
    std::chrono::milliseconds max_catchup_window{5000};
    std::size_t max_batches_per_tick{4};
  };

  SchedulerRuntime(
      Config config,
      std::shared_ptr<leader::LeaseElection> election,
      std::shared_ptr<SchedulerLoop> loop,
      std::shared_ptr<metrics::SchedulerMetrics> metrics,
      std::string scheduler_id);

  // Returns true when dispatch loop ran as leader.
  bool Tick();

  [[nodiscard]] bool IsLeader() const;

 private:
  std::chrono::milliseconds ComputeJitter() const;

  Config config_;
  std::shared_ptr<leader::LeaseElection> election_;
  std::shared_ptr<SchedulerLoop> loop_;
  std::shared_ptr<metrics::SchedulerMetrics> metrics_;
  std::string scheduler_id_;
  bool last_tick_leader_{false};
  mutable std::mt19937 rng_;
};

}  // namespace chronos::scheduler::core
