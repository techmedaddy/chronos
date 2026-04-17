#include "chronos/scheduler/core/scheduler_loop.hpp"

#include <atomic>
#include <optional>
#include <string>
#include <utility>

#include "chronos/scheduler/observability/logger.hpp"
#include "chronos/scheduler/scheduling/misfire_policy.hpp"
#include "chronos/scheduler/scheduling/schedule_calculator.hpp"
#include "chronos/time/clock.hpp"

namespace chronos::scheduler::core {
namespace {

std::string NextExecutionId() {
  static std::atomic<unsigned long long> counter{1};
  return "execution-" + std::to_string(counter.fetch_add(1));
}

std::int64_t NextExecutionNumber() {
  static std::atomic<long long> counter{1};
  return counter.fetch_add(1);
}

}  // namespace

SchedulerLoop::SchedulerLoop(
    Config config,
    std::shared_ptr<persistence::IScheduleRepository> schedule_repository,
    std::shared_ptr<persistence::IExecutionRepository> execution_repository,
    std::shared_ptr<persistence::IOutboxRepository> outbox_repository,
    std::shared_ptr<executor::LocalExecutor> local_executor)
    : config_(std::move(config)),
      schedule_repository_(std::move(schedule_repository)),
      execution_repository_(std::move(execution_repository)),
      outbox_repository_(std::move(outbox_repository)),
      local_executor_(std::move(local_executor)) {}

void SchedulerLoop::Tick() {
  const auto now = time::UtcNow();
  const auto due = schedule_repository_->GetDueSchedules(now, config_.batch_size);

  for (const auto& schedule : due) {
    const auto decision = scheduling::ComputeScheduleDecision(schedule, now, config_.drift_guard);
    if (!decision.due_now || !decision.intended_run_at.has_value()) {
      continue;
    }

    const auto misfire_action = scheduling::ResolveMisfireAction(schedule, decision.misfire_detected);
    if (misfire_action == scheduling::MisfireAction::kSkip) {
      observability::Log("info", "misfire_skipped", "{\"schedule_id\":\"" + schedule.schedule_id + "\"}");
      continue;
    }

    const auto dispatch_key = schedule.schedule_id + "@" + time::ToIso8601(decision.intended_run_at.value());
    if (!duplicate_guard_.TryMarkDispatched(dispatch_key)) {
      observability::Log("warn", "duplicate_dispatch_prevented", "{\"dispatch_key\":\"" + dispatch_key + "\"}");
      continue;
    }

    domain::JobExecution execution;
    execution.execution_id = NextExecutionId();
    execution.job_id = schedule.job_id;
    execution.schedule_id = schedule.schedule_id;
    execution.execution_number = NextExecutionNumber();
    execution.scheduled_at = decision.intended_run_at.value();
    execution.state = domain::ExecutionState::kPending;
    execution.max_attempts = 3;
    execution.created_at = now;
    execution.updated_at = now;

    if (!execution_repository_->CreateExecution(execution)) {
      duplicate_guard_.Clear(dispatch_key);
      continue;
    }

    // Persist dispatch intent before execution publish/pickup.
    domain::OutboxEvent dispatch_event;
    dispatch_event.event_id = "outbox-" + execution.execution_id;
    dispatch_event.aggregate_type = "job_execution";
    dispatch_event.aggregate_id = execution.execution_id;
    dispatch_event.event_type = "execution.dispatch_intent";
    dispatch_event.payload_json = "{\"execution_id\":\"" + execution.execution_id + "\"}";
    dispatch_event.created_at = now;
    outbox_repository_->InsertOutboxEvent(dispatch_event);

    if (!execution_repository_->TransitionExecutionState(
            execution.execution_id,
            domain::ExecutionState::kPending,
            domain::ExecutionState::kDispatched,
            std::nullopt,
            std::nullopt,
            std::string("single-node-scheduler"))) {
      duplicate_guard_.Clear(dispatch_key);
      continue;
    }

    // Phase 2 local flow (no RabbitMQ): run local executor directly.
    local_executor_->Execute(execution.execution_id);
  }
}

}  // namespace chronos::scheduler::core
