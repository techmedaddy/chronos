#include "chronos/worker/observability/logger.hpp"

#include <iostream>

namespace chronos::worker::observability {

void Log(
    const std::string& level,
    const std::string& worker_id,
    const std::string& message,
    const std::string& extra_json) {
  // extra_json should include: job_id, execution_id, attempt, worker_id, trace_id
  std::cout << "{"
            << "\"service\":\"worker\","
            << "\"level\":\"" << level << "\","
            << "\"worker_id\":\"" << worker_id << "\","
            << "\"message\":\"" << message << "\","
            << "\"extra\":" << extra_json
            << "}" << std::endl;
}

}  // namespace chronos::worker::observability
