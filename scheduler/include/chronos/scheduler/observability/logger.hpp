#pragma once

#include <string>

namespace chronos::scheduler::observability {

void Log(
    const std::string& level,
    const std::string& message,
    const std::string& extra_json = "{}");

}  // namespace chronos::scheduler::observability
