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

  return {.status = 200,
          .body = body,
          .headers = {{"content-type", "text/plain; version=0.0.4"}}};
}

}  // namespace chronos::api::handlers
