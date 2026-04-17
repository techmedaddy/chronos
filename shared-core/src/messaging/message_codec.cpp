#include "chronos/messaging/message_codec.hpp"

#include <cctype>
#include <optional>
#include <sstream>
#include <string>

#include "chronos/time/clock.hpp"

namespace chronos::messaging {

std::string EncodeDispatchMessage(const ExecutionDispatchMessage& message) {
  std::ostringstream oss;
  oss << "{";
  oss << "\"message_type\":\"" << message.message_type << "\",";
  oss << "\"message_version\":" << message.message_version << ",";
  oss << "\"trace_id\":\"" << message.trace_id << "\",";
  oss << "\"job_id\":\"" << message.job_id << "\",";
  oss << "\"execution_id\":\"" << message.execution_id << "\",";
  oss << "\"attempt\":" << message.attempt << ",";
  oss << "\"scheduled_at\":\"" << time::ToIso8601(message.scheduled_at) << "\",";
  oss << "\"idempotency_key\":\"" << message.idempotency_key << "\",";
  oss << "\"payload\":" << message.payload_json;
  oss << "}";
  return oss.str();
}

namespace {

std::optional<std::string> ExtractStringField(
    const std::string& body,
    const std::string& key) {
  const auto token = "\"" + key + "\":\"";
  const auto start = body.find(token);
  if (start == std::string::npos) {
    return std::nullopt;
  }
  const auto value_start = start + token.size();
  const auto value_end = body.find('"', value_start);
  if (value_end == std::string::npos) {
    return std::nullopt;
  }
  return body.substr(value_start, value_end - value_start);
}

std::optional<int> ExtractIntField(
    const std::string& body,
    const std::string& key) {
  const auto token = "\"" + key + "\":";
  const auto start = body.find(token);
  if (start == std::string::npos) {
    return std::nullopt;
  }
  const auto value_start = start + token.size();
  auto value_end = value_start;
  while (value_end < body.size() && std::isdigit(static_cast<unsigned char>(body[value_end]))) {
    ++value_end;
  }
  if (value_end == value_start) {
    return std::nullopt;
  }
  return std::stoi(body.substr(value_start, value_end - value_start));
}

std::optional<std::string> ExtractJsonField(
    const std::string& body,
    const std::string& key) {
  const auto token = "\"" + key + "\":";
  const auto start = body.find(token);
  if (start == std::string::npos) {
    return std::nullopt;
  }
  const auto value_start = start + token.size();
  return body.substr(value_start);
}

}  // namespace

std::optional<ExecutionDispatchMessage> DecodeDispatchMessage(const std::string& body) {
  const auto execution_id = ExtractStringField(body, "execution_id");
  const auto job_id = ExtractStringField(body, "job_id");
  const auto trace_id = ExtractStringField(body, "trace_id");
  const auto idempotency_key = ExtractStringField(body, "idempotency_key");
  const auto attempt = ExtractIntField(body, "attempt");
  const auto payload = ExtractJsonField(body, "payload");

  if (!execution_id.has_value() ||
      !job_id.has_value() ||
      !trace_id.has_value() ||
      !idempotency_key.has_value() ||
      !attempt.has_value() ||
      !payload.has_value()) {
    return std::nullopt;
  }

  ExecutionDispatchMessage message;
  message.execution_id = *execution_id;
  message.job_id = *job_id;
  message.trace_id = *trace_id;
  message.idempotency_key = *idempotency_key;
  message.attempt = *attempt;
  message.payload_json = *payload;
  return message;
}

}  // namespace chronos::messaging
