#include "chronos/api/validation/job_validation.hpp"

namespace chronos::api::validation {
namespace {

bool Contains(const std::string& body, const std::string& token) {
  return body.find(token) != std::string::npos;
}

}  // namespace

ValidationResult ValidateCreateJobPayload(const std::string& body) {
  if (!Contains(body, "\"name\"")) {
    return {.ok = false, .message = "name is required"};
  }
  if (!Contains(body, "\"task_type\"")) {
    return {.ok = false, .message = "task_type is required"};
  }
  if (!Contains(body, "\"queue_name\"")) {
    return {.ok = false, .message = "queue_name is required"};
  }
  return {.ok = true, .message = "ok"};
}

ValidationResult ValidateCreateSchedulePayload(const std::string& body) {
  if (!Contains(body, "\"job_id\"")) {
    return {.ok = false, .message = "job_id is required"};
  }
  if (!Contains(body, "\"schedule_type\"")) {
    return {.ok = false, .message = "schedule_type is required"};
  }

  const auto is_cron = Contains(body, "\"CRON\"");
  const auto is_one_time = Contains(body, "\"ONE_TIME\"");

  if (!is_cron && !is_one_time) {
    return {.ok = false, .message = "schedule_type must be CRON or ONE_TIME"};
  }

  if (is_cron && !Contains(body, "\"cron_expression\"")) {
    return {.ok = false, .message = "cron_expression required for CRON"};
  }

  if (is_one_time && !Contains(body, "\"run_at\"")) {
    return {.ok = false, .message = "run_at required for ONE_TIME"};
  }

  return {.ok = true, .message = "ok"};
}

}  // namespace chronos::api::validation
