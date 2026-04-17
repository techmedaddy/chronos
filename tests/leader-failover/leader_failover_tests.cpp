#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include "chronos/messaging/in_memory_queue_broker.hpp"
#include "chronos/persistence/in_memory/in_memory_repositories.hpp"
#include "chronos/scheduler/core/scheduler_loop.hpp"
#include "chronos/scheduler/core/scheduler_runtime.hpp"
#include "chronos/scheduler/executor/local_executor.hpp"
#include "chronos/scheduler/leader/lease_election.hpp"
#include "chronos/scheduler/messaging/rabbitmq_publisher.hpp"
#include "chronos/scheduler/metrics/metrics.hpp"
#include "chronos/time/clock.hpp"

namespace {

using namespace chronos;

struct RuntimeBundle {
  std::shared_ptr<scheduler::core::SchedulerRuntime> runtime;
  std::shared_ptr<scheduler::leader::LeaseElection> election;
  std::shared_ptr<scheduler::metrics::SchedulerMetrics> metrics;
};

RuntimeBundle MakeRuntime(
    const std::string& scheduler_id,
    const std::shared_ptr<scheduler::leader::InMemoryLeaseStore>& lease_store,
    const std::shared_ptr<persistence::in_memory::InMemoryScheduleRepository>& schedules,
    const std::shared_ptr<persistence::in_memory::InMemoryExecutionRepository>& executions,
    const std::shared_ptr<persistence::in_memory::InMemoryOutboxRepository>& outbox,
    const std::shared_ptr<messaging::InMemoryQueueBroker>& broker) {
  auto local_executor = std::make_shared<scheduler::executor::LocalExecutor>(executions);
  auto publisher = std::make_shared<scheduler::messaging::RabbitMqPublisher>(broker);

  scheduler::core::SchedulerLoop::Config loop_cfg;
  loop_cfg.lease_key = "chronos/scheduler/leader";

  auto loop = std::make_shared<scheduler::core::SchedulerLoop>(
      loop_cfg,
      schedules,
      executions,
      outbox,
      local_executor,
      publisher,
      broker,
      lease_store);

  scheduler::leader::LeaseElection::Config ecfg;
  ecfg.lease_key = loop_cfg.lease_key;
  ecfg.ttl = std::chrono::seconds(1);

  auto election = std::make_shared<scheduler::leader::LeaseElection>(ecfg, scheduler_id, lease_store);

  auto metrics = std::make_shared<scheduler::metrics::SchedulerMetrics>();

  scheduler::core::SchedulerRuntime::Config rcfg;
  rcfg.base_loop_interval = std::chrono::milliseconds(5);
  rcfg.max_jitter = std::chrono::milliseconds(0);
  rcfg.max_catchup_window = std::chrono::milliseconds(20);
  rcfg.max_batches_per_tick = 1;

  auto runtime = std::make_shared<scheduler::core::SchedulerRuntime>(
      rcfg,
      election,
      loop,
      metrics,
      scheduler_id);

  return RuntimeBundle{runtime, election, metrics};
}

bool TestLeaderHandoff() {
  auto audit = std::make_shared<persistence::in_memory::InMemoryAuditRepository>();
  auto schedules = std::make_shared<persistence::in_memory::InMemoryScheduleRepository>();
  auto executions = std::make_shared<persistence::in_memory::InMemoryExecutionRepository>(audit);
  auto outbox = std::make_shared<persistence::in_memory::InMemoryOutboxRepository>();
  auto broker = std::make_shared<messaging::InMemoryQueueBroker>();
  auto lease_store = std::make_shared<scheduler::leader::InMemoryLeaseStore>();

  domain::JobSchedule schedule;
  schedule.schedule_id = "sched-failover-1";
  schedule.job_id = "job-failover-1";
  schedule.schedule_type = domain::ScheduleType::kOneTime;
  schedule.next_run_at = time::UtcNow();
  schedule.active = true;
  schedules->UpsertSchedule(schedule);

  auto a = MakeRuntime("scheduler-A", lease_store, schedules, executions, outbox, broker);
  auto b = MakeRuntime("scheduler-B", lease_store, schedules, executions, outbox, broker);

  const auto a_ran = a.runtime->Tick();
  const auto b_ran = b.runtime->Tick();

  // Exactly one should lead initially.
  if (a_ran == b_ran) {
    return false;
  }

  // Let lease expire for current leader.
  std::this_thread::sleep_for(std::chrono::milliseconds(1200));

  const auto a_after = a.runtime->Tick();
  const auto b_after = b.runtime->Tick();

  // At least one should lead after failover cycle.
  return a_after || b_after;
}

bool TestFencingBlocksOldLeader() {
  auto lease_store = std::make_shared<scheduler::leader::InMemoryLeaseStore>();
  scheduler::leader::LeaseElection::Config cfg;
  cfg.lease_key = "chronos/scheduler/leader";
  cfg.ttl = std::chrono::seconds(1);

  scheduler::leader::LeaseElection leader_a(cfg, "scheduler-A", lease_store);
  scheduler::leader::LeaseElection leader_b(cfg, "scheduler-B", lease_store);

  if (!leader_a.Campaign()) {
    return false;
  }

  const auto old_token = leader_a.FenceToken();
  std::this_thread::sleep_for(std::chrono::milliseconds(1200));

  if (!leader_b.Campaign()) {
    return false;
  }

  // A lost lease; keepalive must fail and clear leadership.
  const auto keep = leader_a.KeepAlive();
  if (keep) {
    return false;
  }

  return old_token != leader_b.FenceToken();
}

}  // namespace

int main() {
  const bool handoff_ok = TestLeaderHandoff();
  const bool fencing_ok = TestFencingBlocksOldLeader();

  std::cout << "leader_handoff=" << handoff_ok << "\n";
  std::cout << "fencing_blocks_old_leader=" << fencing_ok << "\n";

  return (handoff_ok && fencing_ok) ? 0 : 1;
}
