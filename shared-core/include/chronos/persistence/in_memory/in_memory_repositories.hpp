#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "chronos/persistence/repository.hpp"

namespace chronos::persistence::in_memory {

class InMemoryAuditRepository final : public IAuditRepository {
 public:
  bool InsertExecutionEvent(
      const std::string& execution_id,
      const std::string& event_type,
      std::optional<domain::ExecutionState> from_state,
      std::optional<domain::ExecutionState> to_state,
      std::optional<std::string> error_code,
      std::optional<std::string> details_json) override;

 private:
  std::mutex mu_;
};

class InMemoryOutboxRepository final : public IOutboxRepository {
 public:
  bool InsertOutboxEvent(const domain::OutboxEvent& event) override;

 private:
  std::mutex mu_;
  std::vector<domain::OutboxEvent> events_;
};

class InMemoryJobRepository final : public IJobRepository {
 public:
  bool CreateJob(const domain::Job& job) override;
  std::optional<domain::Job> GetJobById(const std::string& job_id) override;
  bool UpdateJobState(const std::string& job_id, domain::JobState new_state) override;

 private:
  std::mutex mu_;
  std::unordered_map<std::string, domain::Job> jobs_;
};

class InMemoryScheduleRepository final : public IScheduleRepository {
 public:
  bool UpsertSchedule(const domain::JobSchedule& schedule) override;
  std::vector<domain::JobSchedule> GetDueSchedules(
      domain::Timestamp as_of,
      std::size_t limit) override;

 private:
  std::mutex mu_;
  std::unordered_map<std::string, domain::JobSchedule> schedules_;
};

class InMemoryExecutionRepository final : public IExecutionRepository {
 public:
  explicit InMemoryExecutionRepository(std::shared_ptr<IAuditRepository> audit_repository);

  bool CreateExecution(const domain::JobExecution& execution) override;
  std::optional<domain::JobExecution> GetExecutionById(const std::string& execution_id) override;
  bool TransitionExecutionState(
      const std::string& execution_id,
      domain::ExecutionState expected_from,
      domain::ExecutionState to,
      std::optional<std::string> error_code,
      std::optional<std::string> error_message,
      std::optional<std::string> worker_id) override;
  bool InsertAttempt(const domain::JobAttempt& attempt) override;
  bool UpdateAttemptState(
      const std::string& attempt_id,
      domain::AttemptState state,
      std::optional<std::string> error_code,
      std::optional<std::string> error_message) override;
  bool RecordWorkerHeartbeat(const domain::WorkerHeartbeat& heartbeat) override;
  std::vector<domain::JobExecution> GetExecutionHistoryForJob(
      const std::string& job_id,
      std::size_t limit,
      std::size_t offset) override;

 private:
  std::mutex mu_;
  std::unordered_map<std::string, domain::JobExecution> executions_;
  std::unordered_map<std::string, domain::JobAttempt> attempts_;
  std::vector<domain::WorkerHeartbeat> heartbeats_;
  std::shared_ptr<IAuditRepository> audit_repository_;
};

}  // namespace chronos::persistence::in_memory
