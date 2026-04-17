#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>

namespace chronos::messaging {

struct PublishResult {
  bool accepted{false};
  bool confirm_received{false};
  std::string message_id;
};

struct ConsumedMessage {
  std::string queue_name;
  std::string delivery_tag;
  std::string body;
};

using MessageHandler = std::function<void(const ConsumedMessage&)>;

class IQueueBroker {
 public:
  virtual ~IQueueBroker() = default;

  virtual PublishResult Publish(
      const std::string& queue_name,
      const std::string& body,
      bool require_confirm) = 0;

  virtual std::optional<ConsumedMessage> ConsumeOne(const std::string& queue_name) = 0;

  virtual bool Ack(const std::string& queue_name, const std::string& delivery_tag) = 0;
  virtual bool Nack(
      const std::string& queue_name,
      const std::string& delivery_tag,
      bool requeue,
      const std::string& requeue_to = "") = 0;

  virtual std::size_t QueueDepth(const std::string& queue_name) const = 0;
};

}  // namespace chronos::messaging
