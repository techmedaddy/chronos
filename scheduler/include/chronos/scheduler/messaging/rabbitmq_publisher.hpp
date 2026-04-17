#pragma once

#include <memory>
#include <string>

#include "chronos/messaging/message_codec.hpp"
#include "chronos/messaging/queue_broker.hpp"

namespace chronos::scheduler::messaging {

class RabbitMqPublisher {
 public:
  explicit RabbitMqPublisher(std::shared_ptr<chronos::messaging::IQueueBroker> broker);

  bool PublishDispatch(
      const chronos::messaging::ExecutionDispatchMessage& message,
      const std::string& queue_name,
      bool require_confirm);

 private:
  std::shared_ptr<chronos::messaging::IQueueBroker> broker_;
};

}  // namespace chronos::scheduler::messaging
