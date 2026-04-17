#include "chronos/messaging/in_memory_queue_broker.hpp"

#include <optional>
#include <utility>

namespace chronos::messaging {

PublishResult InMemoryQueueBroker::Publish(
    const std::string& queue_name,
    const std::string& body,
    bool require_confirm) {
  std::lock_guard<std::mutex> lock(mu_);
  const auto delivery_tag = queue_name + "-msg-" + std::to_string(counter_++);

  queues_[queue_name].push_back(BrokerMessage{
      .delivery_tag = delivery_tag,
      .body = body,
      .in_flight = false,
  });

  return PublishResult{
      .accepted = true,
      .confirm_received = require_confirm,
      .message_id = delivery_tag,
  };
}

std::optional<ConsumedMessage> InMemoryQueueBroker::ConsumeOne(const std::string& queue_name) {
  std::lock_guard<std::mutex> lock(mu_);
  auto qit = queues_.find(queue_name);
  if (qit == queues_.end() || qit->second.empty()) {
    return std::nullopt;
  }

  auto message = std::move(qit->second.front());
  qit->second.pop_front();
  message.in_flight = true;
  in_flight_[queue_name][message.delivery_tag] = message;

  return ConsumedMessage{
      .queue_name = queue_name,
      .delivery_tag = message.delivery_tag,
      .body = message.body,
  };
}

bool InMemoryQueueBroker::Ack(const std::string& queue_name, const std::string& delivery_tag) {
  std::lock_guard<std::mutex> lock(mu_);
  auto qit = in_flight_.find(queue_name);
  if (qit == in_flight_.end()) {
    return false;
  }
  return qit->second.erase(delivery_tag) > 0;
}

bool InMemoryQueueBroker::Nack(
    const std::string& queue_name,
    const std::string& delivery_tag,
    bool requeue,
    const std::string& requeue_to) {
  std::lock_guard<std::mutex> lock(mu_);
  auto fit = in_flight_.find(queue_name);
  if (fit == in_flight_.end()) {
    return false;
  }

  auto mit = fit->second.find(delivery_tag);
  if (mit == fit->second.end()) {
    return false;
  }

  auto message = std::move(mit->second);
  fit->second.erase(mit);

  if (requeue) {
    const auto& target = requeue_to.empty() ? queue_name : requeue_to;
    message.in_flight = false;
    queues_[target].push_back(std::move(message));
  }

  return true;
}

std::size_t InMemoryQueueBroker::QueueDepth(const std::string& queue_name) const {
  std::lock_guard<std::mutex> lock(mu_);
  const auto qit = queues_.find(queue_name);
  if (qit == queues_.end()) {
    return 0;
  }
  return qit->second.size();
}

}  // namespace chronos::messaging
