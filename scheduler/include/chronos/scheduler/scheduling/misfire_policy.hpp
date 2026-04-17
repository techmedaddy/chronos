#pragma once

#include "chronos/domain/models.hpp"

namespace chronos::scheduler::scheduling {

enum class MisfireAction {
  kFireNow,
  kSkip,
};

MisfireAction ResolveMisfireAction(
    const domain::JobSchedule& schedule,
    bool misfire_detected);

}  // namespace chronos::scheduler::scheduling
