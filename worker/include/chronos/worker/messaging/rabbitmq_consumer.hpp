#pragma once

#include <memory>
#include <string>

#include "chronos/messaging/message_codec.hpp"
#include "chronos/messaging/queue_broker.hpp"
#include "chronos/worker/runtime/worker_runtime.hpp"

namespace chronos::worker::messaging {

class RabbitMqConsumer {
 public:
  RabbitMqConsumer(
      std::shared_ptr<chronos::messaging::IQueueBroker> broker,
      std::shared_ptr<runtime::WorkerRuntime> runtime,
      std::string worker_id);

  bool ConsumeOnceFromMain();
  bool ConsumeOnceFromRetry();

 private:
  bool ConsumeOnceFromQueue(const std::string& queue_name);

  std::shared_ptr<chronos::messaging::IQueueBroker> broker_;
  std::shared_ptr<runtime::WorkerRuntime> runtime_;
  std::string worker_id_;
};

}  // namespace chronos::worker::messaging
