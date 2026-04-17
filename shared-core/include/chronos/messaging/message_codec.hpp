#pragma once

#include <optional>
#include <string>

#include "chronos/domain/models.hpp"

namespace chronos::messaging {

inline constexpr const char* kMainQueue = "main_queue";
inline constexpr const char* kRetryQueue = "retry_queue";
inline constexpr const char* kDeadLetterQueue = "dead_letter_queue";

struct ExecutionDispatchMessage {
  int message_version{1};
  std::string message_type{"execution.dispatch"};
  std::string trace_id;
  std::string job_id;
  std::string execution_id;
  int attempt{1};
  domain::Timestamp scheduled_at{};
  std::string idempotency_key;
  std::string payload_json;
};

struct DecodedMessageEnvelope {
  std::string queue_name;
  std::string raw_message;
  std::string delivery_tag;
};

std::string EncodeDispatchMessage(const ExecutionDispatchMessage& message);
std::optional<ExecutionDispatchMessage> DecodeDispatchMessage(const std::string& body);

}  // namespace chronos::messaging
