#pragma once

#include <optional>

#include "chronos/domain/models.hpp"

namespace chronos::scheduler::scheduling {

struct ScheduleDecision {
  bool due_now{false};
  bool misfire_detected{false};
  std::optional<domain::Timestamp> next_run_at;
  std::optional<domain::Timestamp> intended_run_at;
};

ScheduleDecision ComputeScheduleDecision(
    const domain::JobSchedule& schedule,
    domain::Timestamp now,
    std::chrono::seconds drift_guard);

}  // namespace chronos::scheduler::scheduling
