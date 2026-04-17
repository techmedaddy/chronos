#include "chronos/scheduler/scheduling/schedule_calculator.hpp"

#include <ctime>
#include <sstream>

namespace chronos::scheduler::scheduling {
namespace {

std::optional<domain::Timestamp> ParseSimpleCronNext(
    const std::string& cron_expression,
    domain::Timestamp now) {
  // Minimal deterministic parser for patterns: "M H * * *"
  // Any invalid expression returns +60s fallback.
  std::stringstream ss(cron_expression);
  int minute = -1;
  int hour = -1;
  std::string d1, d2, d3;
  if (!(ss >> minute >> hour >> d1 >> d2 >> d3)) {
    return now + std::chrono::minutes(1);
  }

  if (minute < 0 || minute > 59 || hour < 0 || hour > 23) {
    return now + std::chrono::minutes(1);
  }

  auto candidate = now;
  candidate -= std::chrono::seconds(
      std::chrono::duration_cast<std::chrono::seconds>(candidate.time_since_epoch()).count() % 60);

  for (int i = 0; i < 60 * 48; ++i) {
    auto tt = std::chrono::system_clock::to_time_t(candidate);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &tt);
#else
    gmtime_r(&tt, &tm);
#endif
    if (tm.tm_hour == hour && tm.tm_min == minute) {
      if (candidate > now) {
        return candidate;
      }
    }
    candidate += std::chrono::minutes(1);
  }

  return now + std::chrono::minutes(1);
}

}  // namespace

ScheduleDecision ComputeScheduleDecision(
    const domain::JobSchedule& schedule,
    domain::Timestamp now,
    std::chrono::seconds drift_guard) {
  ScheduleDecision decision;

  if (!schedule.active || !schedule.next_run_at.has_value()) {
    return decision;
  }

  const auto next_run = schedule.next_run_at.value();

  if (now + drift_guard < next_run) {
    return decision;
  }

  decision.due_now = true;
  decision.intended_run_at = next_run;

  if (now - next_run > drift_guard) {
    decision.misfire_detected = true;
  }

  if (schedule.schedule_type == domain::ScheduleType::kOneTime) {
    decision.next_run_at = std::nullopt;
    return decision;
  }

  if (schedule.cron_expression.has_value()) {
    decision.next_run_at = ParseSimpleCronNext(schedule.cron_expression.value(), now);
  } else {
    decision.next_run_at = now + std::chrono::minutes(1);
  }

  return decision;
}

}  // namespace chronos::scheduler::scheduling
