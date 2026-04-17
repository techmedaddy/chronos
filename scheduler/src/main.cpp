#include <iostream>
#include <memory>

#include "chronos/messaging/in_memory_queue_broker.hpp"
#include "chronos/persistence/in_memory/in_memory_repositories.hpp"
#include "chronos/scheduler/core/scheduler_loop.hpp"
#include "chronos/scheduler/core/scheduler_runtime.hpp"
#include "chronos/scheduler/executor/local_executor.hpp"
#include "chronos/scheduler/leader/lease_election.hpp"
#include "chronos/scheduler/messaging/rabbitmq_publisher.hpp"
#include "chronos/scheduler/metrics/metrics.hpp"
#include "chronos/time/clock.hpp"

int main() {
  using namespace chronos;

  auto audit = std::make_shared<persistence::in_memory::InMemoryAuditRepository>();
  auto outbox = std::make_shared<persistence::in_memory::InMemoryOutboxRepository>();
  auto schedules = std::make_shared<persistence::in_memory::InMemoryScheduleRepository>();
  auto executions = std::make_shared<persistence::in_memory::InMemoryExecutionRepository>(audit);
  auto local_executor = std::make_shared<scheduler::executor::LocalExecutor>(executions);
  auto broker = std::make_shared<messaging::InMemoryQueueBroker>();
  auto publisher = std::make_shared<scheduler::messaging::RabbitMqPublisher>(broker);
  auto lease_store = std::make_shared<scheduler::leader::InMemoryLeaseStore>();

  domain::JobSchedule schedule;
  schedule.schedule_id = "sched-1";
  schedule.job_id = "job-1";
  schedule.schedule_type = domain::ScheduleType::kOneTime;
  schedule.next_run_at = time::UtcNow();
  schedule.active = true;
  schedule.misfire_policy = "FIRE_ONCE";
  schedules->UpsertSchedule(schedule);

  scheduler::core::SchedulerLoop::Config loop_config;
  auto loop = std::make_shared<scheduler::core::SchedulerLoop>(
      loop_config,
      schedules,
      executions,
      outbox,
      local_executor,
      publisher,
      broker,
      lease_store);

  auto metrics = std::make_shared<scheduler::metrics::SchedulerMetrics>();

  scheduler::leader::LeaseElection::Config election_config;
  election_config.lease_key = loop_config.lease_key;
  election_config.ttl = std::chrono::seconds(10);

  auto election = std::make_shared<scheduler::leader::LeaseElection>(
      election_config,
      "scheduler-a",
      lease_store);

  scheduler::core::SchedulerRuntime::Config runtime_config;
  runtime_config.base_loop_interval = std::chrono::milliseconds(10);
  runtime_config.max_jitter = std::chrono::milliseconds(5);
  runtime_config.max_catchup_window = std::chrono::milliseconds(100);
  runtime_config.max_batches_per_tick = 2;

  scheduler::core::SchedulerRuntime runtime(
      runtime_config,
      election,
      loop,
      metrics,
      "scheduler-a");

  runtime.Tick();

  std::cout << "chronos-scheduler distributed runtime tick complete\n";
  std::cout << scheduler::metrics::ToPrometheus(*metrics);
  return 0;
}
