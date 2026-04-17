#include "chronos/worker/messaging/rabbitmq_consumer.hpp"

#include <optional>
#include <string>
#include <utility>

namespace chronos::worker::messaging {

RabbitMqConsumer::RabbitMqConsumer(
    std::shared_ptr<chronos::messaging::IQueueBroker> broker,
    std::shared_ptr<runtime::WorkerRuntime> runtime,
    std::string worker_id)
    : broker_(std::move(broker)),
      runtime_(std::move(runtime)),
      worker_id_(std::move(worker_id)) {}

bool RabbitMqConsumer::ConsumeOnceFromMain() {
  return ConsumeOnceFromQueue(chronos::messaging::kMainQueue);
}

bool RabbitMqConsumer::ConsumeOnceFromRetry() {
  return ConsumeOnceFromQueue(chronos::messaging::kRetryQueue);
}

bool RabbitMqConsumer::ConsumeOnceFromQueue(const std::string& queue_name) {
  const auto consumed = broker_->ConsumeOne(queue_name);
  if (!consumed.has_value()) {
    return false;
  }

  const auto decoded = chronos::messaging::DecodeDispatchMessage(consumed->body);
  if (!decoded.has_value()) {
    broker_->Nack(
        queue_name,
        consumed->delivery_tag,
        true,
        chronos::messaging::kDeadLetterQueue);
    return false;
  }

  // DB must remain source of truth:
  // runtime claim+state transitions happen before ACK.
  const auto submitted = runtime_->SubmitExecution(
      decoded->execution_id,
      "sample.echo",
      decoded->payload_json,
      decoded->idempotency_key,
      3);

  if (submitted) {
    return broker_->Ack(queue_name, consumed->delivery_tag);
  }

  // Claim failed or runtime rejected; route to retry queue.
  return broker_->Nack(
      queue_name,
      consumed->delivery_tag,
      true,
      chronos::messaging::kRetryQueue);
}

}  // namespace chronos::worker::messaging
