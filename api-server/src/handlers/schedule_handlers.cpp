#include "chronos/api/handlers/schedule_handlers.hpp"

#include <atomic>

#include "chronos/api/validation/job_validation.hpp"
#include "chronos/time/clock.hpp"

namespace chronos::api::handlers {
namespace {

std::string NextScheduleId() {
  static std::atomic<unsigned long long> counter{1};
  return "schedule-" + std::to_string(counter.fetch_add(1));
}

}  // namespace

http::HttpResponse HandleCreateSchedule(
    const http::HttpRequest& request,
    const std::shared_ptr<HandlerContext>& context) {
  context->metrics->requests_total.fetch_add(1);

  const auto validation = validation::ValidateCreateSchedulePayload(request.body);
  if (!validation.ok) {
    return {.status = 400,
            .body = "{\"error\":{\"code\":\"VALIDATION_ERROR\",\"message\":\"" +
                    validation.message + "\"}}",
            .headers = {{"content-type", "application/json"}}};
  }

  domain::JobSchedule schedule;
  schedule.schedule_id = NextScheduleId();
  schedule.job_id = "unknown-job-from-body";
  schedule.schedule_type = request.body.find("CRON") != std::string::npos
                               ? domain::ScheduleType::kCron
                               : domain::ScheduleType::kOneTime;
  schedule.timezone = "UTC";
  schedule.misfire_policy = "FIRE_ONCE";
  schedule.created_at = time::UtcNow();
  schedule.updated_at = schedule.created_at;
  schedule.next_run_at = time::UtcNow();

  if (!context->schedule_repository->UpsertSchedule(schedule)) {
    return {.status = 500,
            .body = R"({"error":{"code":"INTERNAL_ERROR","message":"failed to persist schedule"}})",
            .headers = {{"content-type", "application/json"}}};
  }

  context->metrics->schedules_created_total.fetch_add(1);

  return {.status = 201,
          .body = "{\"schedule_id\":\"" + schedule.schedule_id +
                  "\",\"active\":true}",
          .headers = {{"content-type", "application/json"}}};
}

}  // namespace chronos::api::handlers
