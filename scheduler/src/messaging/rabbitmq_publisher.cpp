#include "chronos/scheduler/messaging/rabbitmq_publisher.hpp"

#include <utility>

namespace chronos::scheduler::messaging {

RabbitMqPublisher::RabbitMqPublisher(
    std::shared_ptr<chronos::messaging::IQueueBroker> broker)
    : broker_(std::move(broker)) {}

bool RabbitMqPublisher::PublishDispatch(
    const chronos::messaging::ExecutionDispatchMessage& message,
    const std::string& queue_name,
    bool require_confirm) {
  const auto body = chronos::messaging::EncodeDispatchMessage(message);
  const auto result = broker_->Publish(queue_name, body, require_confirm);
  return result.accepted && (!require_confirm || result.confirm_received);
}

}  // namespace chronos::scheduler::messaging
