#include "chronos/api/observability/logger.hpp"

#include <iostream>

namespace chronos::api::observability {
namespace {

std::string ToString(LogLevel level) {
  switch (level) {
    case LogLevel::kDebug:
      return "debug";
    case LogLevel::kInfo:
      return "info";
    case LogLevel::kWarn:
      return "warn";
    case LogLevel::kError:
      return "error";
  }
  return "info";
}

}  // namespace

void Log(
    LogLevel level,
    const std::string& service,
    const std::string& request_id,
    const std::string& message,
    const std::string& extra_json) {
  std::cout << "{"
            << "\"level\":\"" << ToString(level) << "\","
            << "\"service\":\"" << service << "\","
            << "\"request_id\":\"" << request_id << "\","
            << "\"message\":\"" << message << "\","
            << "\"extra\":" << extra_json
            << "}" << std::endl;
}

}  // namespace chronos::api::observability
