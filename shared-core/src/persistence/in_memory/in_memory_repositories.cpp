#include "chronos/persistence/in_memory/in_memory_repositories.hpp"

#include <algorithm>
#include <cstddef>
#include <mutex>
#include <optional>
#include <utility>

#include "chronos/state_machine/execution_state_machine.hpp"
#include "chronos/time/clock.hpp"

namespace chronos::persistence::in_memory {

bool InMemoryAuditRepository::InsertExecutionEvent(
    const std::string& execution_id,
    const std::string& event_type,
    std::optional<domain::ExecutionState> from_state,
    std::optional<domain::ExecutionState> to_state,
    std::optional<std::string> error_code,
    std::optional<std::string> details_json) {
  (void)execution_id;
  (void)event_type;
  (void)from_state;
  (void)to_state;
  (void)error_code;
  (void)details_json;
  std::lock_guard<std::mutex> lock(mu_);
  return true;
}

bool InMemoryOutboxRepository::InsertOutboxEvent(const domain::OutboxEvent& event) {
  std::lock_guard<std::mutex> lock(mu_);
  events_.push_back(event);
  return true;
}

bool InMemoryJobRepository::CreateJob(const domain::Job& job) {
  std::lock_guard<std::mutex> lock(mu_);
  const auto [_, inserted] = jobs_.emplace(job.job_id, job);
  return inserted;
}

std::optional<domain::Job> InMemoryJobRepository::GetJobById(const std::string& job_id) {
  std::lock_guard<std::mutex> lock(mu_);
  const auto it = jobs_.find(job_id);
  if (it == jobs_.end()) {
    return std::nullopt;
  }
  return it->second;
}

bool InMemoryJobRepository::UpdateJobState(
    const std::string& job_id,
    domain::JobState new_state) {
  std::lock_guard<std::mutex> lock(mu_);
  const auto it = jobs_.find(job_id);
  if (it == jobs_.end()) {
    return false;
  }
  it->second.state = new_state;
  it->second.updated_at = time::UtcNow();
  return true;
}

bool InMemoryScheduleRepository::UpsertSchedule(const domain::JobSchedule& schedule) {
  std::lock_guard<std::mutex> lock(mu_);
  schedules_[schedule.schedule_id] = schedule;
  return true;
}

std::vector<domain::JobSchedule> InMemoryScheduleRepository::GetDueSchedules(
    domain::Timestamp as_of,
    std::size_t limit) {
  std::lock_guard<std::mutex> lock(mu_);
  std::vector<domain::JobSchedule> due;
  due.reserve(limit);

  for (const auto& [_, schedule] : schedules_) {
    if (!schedule.active || !schedule.next_run_at.has_value()) {
      continue;
    }
    if (schedule.next_run_at.value() <= as_of) {
      due.push_back(schedule);
      if (due.size() >= limit) {
        break;
      }
    }
  }

  return due;
}

InMemoryExecutionRepository::InMemoryExecutionRepository(
    std::shared_ptr<IAuditRepository> audit_repository)
    : audit_repository_(std::move(audit_repository)) {}

bool InMemoryExecutionRepository::CreateExecution(const domain::JobExecution& execution) {
  std::lock_guard<std::mutex> lock(mu_);
  const auto [_, inserted] = executions_.emplace(execution.execution_id, execution);
  return inserted;
}

std::optional<domain::JobExecution> InMemoryExecutionRepository::GetExecutionById(
    const std::string& execution_id) {
  std::lock_guard<std::mutex> lock(mu_);
  const auto it = executions_.find(execution_id);
  if (it == executions_.end()) {
    return std::nullopt;
  }
  return it->second;
}

bool InMemoryExecutionRepository::TransitionExecutionState(
    const std::string& execution_id,
    domain::ExecutionState expected_from,
    domain::ExecutionState to,
    std::optional<std::string> error_code,
    std::optional<std::string> error_message,
    std::optional<std::string> worker_id) {
  std::lock_guard<std::mutex> lock(mu_);
  const auto it = executions_.find(execution_id);
  if (it == executions_.end()) {
    return false;
  }

  if (it->second.state != expected_from) {
    return false;
  }

  const auto decision = state_machine::ValidateExecutionTransition(expected_from, to);
  if (!decision.allowed) {
    return false;
  }

  it->second.state = to;
  it->second.updated_at = time::UtcNow();
  it->second.last_error_code = std::move(error_code);
  it->second.last_error_message = std::move(error_message);
  it->second.current_worker_id = std::move(worker_id);

  if (to == domain::ExecutionState::kRunning) {
    it->second.started_at = time::UtcNow();
    it->second.attempt_count += 1;
  }

  if (to == domain::ExecutionState::kSucceeded ||
      to == domain::ExecutionState::kFailed ||
      to == domain::ExecutionState::kDeadLetter) {
    it->second.finished_at = time::UtcNow();
  }

  if (audit_repository_) {
    audit_repository_->InsertExecutionEvent(
        execution_id,
        "execution.state_transition",
        expected_from,
        to,
        it->second.last_error_code,
        std::nullopt);
  }

  return true;
}

bool InMemoryExecutionRepository::InsertAttempt(const domain::JobAttempt& attempt) {
  std::lock_guard<std::mutex> lock(mu_);
  const auto [_, inserted] = attempts_.emplace(attempt.attempt_id, attempt);
  return inserted;
}

bool InMemoryExecutionRepository::UpdateAttemptState(
    const std::string& attempt_id,
    domain::AttemptState state,
    std::optional<std::string> error_code,
    std::optional<std::string> error_message) {
  std::lock_guard<std::mutex> lock(mu_);
  const auto it = attempts_.find(attempt_id);
  if (it == attempts_.end()) {
    return false;
  }

  it->second.state = state;
  it->second.error_code = std::move(error_code);
  it->second.error_message = std::move(error_message);
  it->second.updated_at = time::UtcNow();

  if (state == domain::AttemptState::kSucceeded ||
      state == domain::AttemptState::kFailed ||
      state == domain::AttemptState::kTimedOut ||
      state == domain::AttemptState::kCancelled) {
    it->second.finished_at = time::UtcNow();
  }

  return true;
}

bool InMemoryExecutionRepository::RecordWorkerHeartbeat(
    const domain::WorkerHeartbeat& heartbeat) {
  std::lock_guard<std::mutex> lock(mu_);
  heartbeats_.push_back(heartbeat);

  const auto it = executions_.find(heartbeat.execution_id);
  if (it != executions_.end()) {
    it->second.last_heartbeat_at = heartbeat.heartbeat_at;
  }

  return true;
}

std::vector<domain::JobExecution> InMemoryExecutionRepository::GetExecutionHistoryForJob(
    const std::string& job_id,
    std::size_t limit,
    std::size_t offset) {
  std::lock_guard<std::mutex> lock(mu_);

  std::vector<domain::JobExecution> all;
  all.reserve(executions_.size());
  for (const auto& [_, execution] : executions_) {
    if (execution.job_id == job_id) {
      all.push_back(execution);
    }
  }

  std::sort(
      all.begin(),
      all.end(),
      [](const domain::JobExecution& lhs, const domain::JobExecution& rhs) {
        return lhs.execution_number > rhs.execution_number;
      });

  if (offset >= all.size()) {
    return {};
  }

  const auto end = std::min(all.size(), offset + limit);
  return {all.begin() + static_cast<std::ptrdiff_t>(offset),
          all.begin() + static_cast<std::ptrdiff_t>(end)};
}

}  // namespace chronos::persistence::in_memory
