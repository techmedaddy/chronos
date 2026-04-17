#include "chronos/scheduler/scheduling/misfire_policy.hpp"

namespace chronos::scheduler::scheduling {

MisfireAction ResolveMisfireAction(
    const domain::JobSchedule& schedule,
    bool misfire_detected) {
  if (!misfire_detected) {
    return MisfireAction::kFireNow;
  }

  if (schedule.misfire_policy == "SKIP") {
    return MisfireAction::kSkip;
  }

  // FIRE_ONCE default
  return MisfireAction::kFireNow;
}

}  // namespace chronos::scheduler::scheduling
