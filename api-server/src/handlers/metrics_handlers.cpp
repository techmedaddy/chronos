#include "chronos/api/handlers/metrics_handlers.hpp"

namespace chronos::api::handlers {

http::HttpResponse HandleMetrics(
    const http::HttpRequest& request,
    const std::shared_ptr<HandlerContext>& context) {
  (void)request;

  std::string body;
  body += "# TYPE chronos_api_requests_total counter\n";
  body += "chronos_api_requests_total " +
          std::to_string(context->metrics->requests_total.load()) + "\n";
  body += "# TYPE chronos_api_jobs_created_total counter\n";
  body += "chronos_api_jobs_created_total " +
          std::to_string(context->metrics->jobs_created_total.load()) + "\n";
  body += "# TYPE chronos_api_schedules_created_total counter\n";
  body += "chronos_api_schedules_created_total " +
          std::to_string(context->metrics->schedules_created_total.load()) + "\n";
  body += "# TYPE chronos_api_execution_queries_total counter\n";
  body += "chronos_api_execution_queries_total " +
          std::to_string(context->metrics->execution_queries_total.load()) + "\n";

  // Phase 8 metrics
  body += "chronos_schedule_lag_ms " + std::to_string(context->metrics->schedule_lag_ms.load()) + "\n";
  body += "chronos_queue_depth " + std::to_string(context->metrics->queue_depth.load()) + "\n";
  body += "chronos_dispatch_rate_total " + std::to_string(context->metrics->dispatch_rate_total.load()) + "\n";
  body += "chronos_success_total " + std::to_string(context->metrics->success_total.load()) + "\n";
  body += "chronos_failure_total " + std::to_string(context->metrics->failure_total.load()) + "\n";
  body += "chronos_retry_total " + std::to_string(context->metrics->retry_total.load()) + "\n";
  body += "chronos_worker_utilization_pct " + std::to_string(context->metrics->worker_utilization_pct.load()) + "\n";
  body += "chronos_task_latency_ms " + std::to_string(context->metrics->task_latency_ms.load()) + "\n";
  body += "chronos_db_latency_ms " + std::to_string(context->metrics->db_latency_ms.load()) + "\n";

  return {.status = 200,
          .body = body,
          .headers = {{"content-type", "text/plain; version=0.0.4"}}};
}

}  // namespace chronos::api::handlers
