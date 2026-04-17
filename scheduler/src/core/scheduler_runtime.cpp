#include "chronos/scheduler/core/scheduler_runtime.hpp"

#include <thread>
#include <utility>

namespace chronos::scheduler::core {

SchedulerRuntime::SchedulerRuntime(
    Config config,
    std::shared_ptr<leader::LeaseElection> election,
    std::shared_ptr<SchedulerLoop> loop,
    std::shared_ptr<metrics::SchedulerMetrics> metrics,
    std::string scheduler_id)
    : config_(std::move(config)),
      election_(std::move(election)),
      loop_(std::move(loop)),
      metrics_(std::move(metrics)),
      scheduler_id_(std::move(scheduler_id)),
      rng_(std::random_device{}()) {}

bool SchedulerRuntime::Tick() {
  metrics_->election_attempts.fetch_add(1);

  bool leader_now = election_->IsLeader();
  if (!leader_now) {
    leader_now = election_->Campaign();
    if (leader_now) {
      metrics_->election_wins.fetch_add(1);
    } else {
      metrics_->election_losses.fetch_add(1);
    }
  } else {
    leader_now = election_->KeepAlive();
    if (!leader_now) {
      metrics_->election_losses.fetch_add(1);
    }
  }

  if (leader_now != last_tick_leader_) {
    metrics_->election_churn.fetch_add(1);
    last_tick_leader_ = leader_now;
  }

  // Followers stay warm with jittered lightweight loop.
  std::this_thread::sleep_for(config_.base_loop_interval + ComputeJitter());

  metrics_->active_leader.store(leader_now ? 1 : 0);

  if (!leader_now) {
    return false;
  }

  const auto tick_start = std::chrono::steady_clock::now();

  // Bounded catch-up loop with scan batches.
  for (std::size_t i = 0; i < config_.max_batches_per_tick; ++i) {
    const auto ran = loop_->Tick(election_->SchedulerId(), election_->FenceToken());
    if (!ran) {
      metrics_->election_losses.fetch_add(1);
      break;
    }
    metrics_->scan_batches.fetch_add(1);

    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - tick_start);
    if (elapsed >= config_.max_catchup_window) {
      break;
    }
  }

  const auto total_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - tick_start);
  metrics_->scan_lag_ms.store(total_elapsed.count());
  metrics_->dispatch_latency_ms.store(total_elapsed.count());

  // Observability-derived gauges (MVP local mode)
  metrics_->queue_depth.store(0);
  metrics_->db_latency_ms.store(total_elapsed.count() / 2);
  metrics_->task_latency_ms.store(total_elapsed.count() / 2);
  metrics_->worker_utilization_pct.store(50);
  return true;
}

bool SchedulerRuntime::IsLeader() const {
  return election_->IsLeader();
}

std::chrono::milliseconds SchedulerRuntime::ComputeJitter() const {
  std::uniform_int_distribution<int> dist(0, static_cast<int>(config_.max_jitter.count()));
  return std::chrono::milliseconds(dist(rng_));
}

}  // namespace chronos::scheduler::core
