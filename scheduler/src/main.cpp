#include <iostream>
#include <memory>

#include "chronos/messaging/in_memory_queue_broker.hpp"
#include "chronos/messaging/message_codec.hpp"
#include "chronos/persistence/in_memory/in_memory_repositories.hpp"
#include "chronos/scheduler/core/scheduler_loop.hpp"
#include "chronos/scheduler/executor/local_executor.hpp"
#include "chronos/scheduler/messaging/rabbitmq_publisher.hpp"
#include "chronos/scheduler/reconciliation/reconciliation_job.hpp"
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
  auto reconciler = std::make_shared<scheduler::reconciliation::ReconciliationJob>(
      executions,
      publisher,
      broker);

  domain::JobSchedule schedule;
  schedule.schedule_id = "sched-1";
  schedule.job_id = "job-1";
  schedule.schedule_type = domain::ScheduleType::kOneTime;
  schedule.next_run_at = time::UtcNow();
  schedule.active = true;
  schedule.misfire_policy = "FIRE_ONCE";
  schedules->UpsertSchedule(schedule);

  scheduler::core::SchedulerLoop::Config config;
  scheduler::core::SchedulerLoop loop(
      config,
      schedules,
      executions,
      outbox,
      local_executor,
      publisher,
      broker);

  loop.Tick();

  // Simulate queue drift: message disappears while execution remains DISPATCHED.
  const auto consumed = broker->ConsumeOne(messaging::kMainQueue);
  if (consumed.has_value()) {
    broker->Ack(messaging::kMainQueue, consumed->delivery_tag);
  }

  const auto repaired = reconciler->DetectAndRepair({"execution-1"}, "trace-reconcile-1");
  std::cout << "repaired_count=" << repaired.size() << std::endl;

  std::cout << "chronos-scheduler ran one tick (publisher confirm + drift reconciliation mode)" << std::endl;
  return 0;
}
