#pragma once

#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>

#include "chronos/messaging/queue_broker.hpp"

namespace chronos::messaging {

class InMemoryQueueBroker final : public IQueueBroker {
 public:
  PublishResult Publish(
      const std::string& queue_name,
      const std::string& body,
      bool require_confirm) override;

  std::optional<ConsumedMessage> ConsumeOne(const std::string& queue_name) override;

  bool Ack(const std::string& queue_name, const std::string& delivery_tag) override;
  bool Nack(
      const std::string& queue_name,
      const std::string& delivery_tag,
      bool requeue,
      const std::string& requeue_to) override;

  std::size_t QueueDepth(const std::string& queue_name) const override;

 private:
  struct BrokerMessage {
    std::string delivery_tag;
    std::string body;
    bool in_flight{false};
  };

  mutable std::mutex mu_;
  std::unordered_map<std::string, std::deque<BrokerMessage>> queues_;
  std::unordered_map<std::string, std::unordered_map<std::string, BrokerMessage>> in_flight_;
  unsigned long long counter_{1};
};

}  // namespace chronos::messaging
