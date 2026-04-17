#pragma once

#include <string>

namespace chronos::api::observability {

enum class LogLevel {
  kDebug,
  kInfo,
  kWarn,
  kError,
};

void Log(
    LogLevel level,
    const std::string& service,
    const std::string& request_id,
    const std::string& message,
    const std::string& extra_json = "{}");

}  // namespace chronos::api::observability
