#include "chronos/scheduler/reconciliation/reconciliation_job.hpp"

#include <utility>

#include "chronos/messaging/message_codec.hpp"
#include "chronos/time/clock.hpp"

namespace chronos::scheduler::reconciliation {

ReconciliationJob::ReconciliationJob(
    std::shared_ptr<persistence::IExecutionRepository> execution_repository,
    std::shared_ptr<messaging::RabbitMqPublisher> publisher,
    std::shared_ptr<chronos::messaging::IQueueBroker> broker)
    : execution_repository_(std::move(execution_repository)),
      publisher_(std::move(publisher)),
      broker_(std::move(broker)) {}

std::vector<ReconciliationJob::DriftRecord> ReconciliationJob::DetectAndRepair(
    const std::vector<std::string>& expected_inflight_execution_ids,
    const std::string& trace_id) {
  std::vector<DriftRecord> repaired;

  // MVP drift detection:
  // if main_queue empty while DB says there should be inflight executions,
  // republish dispatch messages.
  const auto queue_depth = broker_->QueueDepth(chronos::messaging::kMainQueue);
  if (queue_depth > 0) {
    return repaired;
  }

  for (const auto& execution_id : expected_inflight_execution_ids) {
    const auto execution = execution_repository_->GetExecutionById(execution_id);
    if (!execution.has_value()) {
      continue;
    }

    if (execution->state != domain::ExecutionState::kDispatched &&
        execution->state != domain::ExecutionState::kRetryPending) {
      continue;
    }

    chronos::messaging::ExecutionDispatchMessage message;
    message.trace_id = trace_id;
    message.job_id = execution->job_id;
    message.execution_id = execution->execution_id;
    message.attempt = execution->attempt_count + 1;
    message.scheduled_at = execution->scheduled_at;
    message.idempotency_key = execution->execution_id;
    message.payload_json = execution->result_summary_json.value_or("{}");

    const auto published = publisher_->PublishDispatch(
        message,
        chronos::messaging::kMainQueue,
        true);

    if (published) {
      repaired.push_back(DriftRecord{
          .execution_id = execution->execution_id,
          .reason = "queue_drift_republished",
      });
    }
  }

  return repaired;
}

}  // namespace chronos::scheduler::reconciliation
