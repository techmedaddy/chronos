#include <iostream>
#include <memory>

#include "chronos/persistence/in_memory/in_memory_repositories.hpp"
#include "chronos/scheduler/core/scheduler_loop.hpp"
#include "chronos/scheduler/executor/local_executor.hpp"
#include "chronos/time/clock.hpp"

int main() {
  using namespace chronos;

  auto audit = std::make_shared<persistence::in_memory::InMemoryAuditRepository>();
  auto outbox = std::make_shared<persistence::in_memory::InMemoryOutboxRepository>();
  auto schedules = std::make_shared<persistence::in_memory::InMemoryScheduleRepository>();
  auto executions = std::make_shared<persistence::in_memory::InMemoryExecutionRepository>(audit);
  auto local_executor = std::make_shared<scheduler::executor::LocalExecutor>(executions);

  domain::JobSchedule schedule;
  schedule.schedule_id = "sched-1";
  schedule.job_id = "job-1";
  schedule.schedule_type = domain::ScheduleType::kOneTime;
  schedule.next_run_at = time::UtcNow();
  schedule.active = true;
  schedule.misfire_policy = "FIRE_ONCE";
  schedules->UpsertSchedule(schedule);

  scheduler::core::SchedulerLoop::Config config;
  scheduler::core::SchedulerLoop loop(config, schedules, executions, outbox, local_executor);

  loop.Tick();

  std::cout << "chronos-scheduler ran one tick (single-node local-executor mode)" << std::endl;
  return 0;
}
