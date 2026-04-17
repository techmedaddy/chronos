#include "chronos/scheduler/observability/logger.hpp"

#include <iostream>

namespace chronos::scheduler::observability {

void Log(
    const std::string& level,
    const std::string& message,
    const std::string& extra_json) {
  std::cout << "{"
            << "\"service\":\"scheduler\","
            << "\"level\":\"" << level << "\","
            << "\"message\":\"" << message << "\","
            << "\"extra\":" << extra_json
            << "}" << std::endl;
}

}  // namespace chronos::scheduler::observability
