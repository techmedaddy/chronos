#pragma once

#include <string>

namespace chronos::api::validation {

struct ValidationResult {
  bool ok{true};
  std::string message;
};

ValidationResult ValidateCreateJobPayload(const std::string& body);
ValidationResult ValidateCreateSchedulePayload(const std::string& body);

}  // namespace chronos::api::validation
