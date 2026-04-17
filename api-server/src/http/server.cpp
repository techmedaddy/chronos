#include "chronos/api/http/server.hpp"

#include <utility>

#include "chronos/api/handlers/health_handlers.hpp"
#include "chronos/api/handlers/job_handlers.hpp"
#include "chronos/api/handlers/metrics_handlers.hpp"
#include "chronos/api/handlers/schedule_handlers.hpp"
#include "chronos/api/middleware/request_id_middleware.hpp"
#include "chronos/api/observability/logger.hpp"

namespace chronos::api::http {

ApiServer::ApiServer(
    std::shared_ptr<handlers::HandlerContext> context,
    std::string bearer_token)
    : context_(std::move(context)),
      auth_middleware_(std::move(bearer_token)) {
  router_.Register("POST", "/jobs", [ctx = context_](const HttpRequest& req) {
    return handlers::HandleCreateJob(req, ctx);
  });

  router_.Register("POST", "/schedules", [ctx = context_](const HttpRequest& req) {
    return handlers::HandleCreateSchedule(req, ctx);
  });

  router_.Register("GET", "/jobs/:id", [ctx = context_](const HttpRequest& req) {
    return handlers::HandleGetJobById(req, ctx);
  });

  router_.Register("GET", "/jobs/:id/executions", [ctx = context_](const HttpRequest& req) {
    return handlers::HandleGetJobExecutions(req, ctx);
  });

  router_.Register("GET", "/health", [](const HttpRequest& req) {
    return handlers::HandleHealth(req);
  });

  router_.Register("GET", "/metrics", [ctx = context_](const HttpRequest& req) {
    return handlers::HandleMetrics(req, ctx);
  });
}

HttpResponse ApiServer::Handle(HttpRequest request) const {
  request.request_id = middleware::RequestIdMiddleware::ResolveOrGenerate(request);

  observability::Log(
      observability::LogLevel::kInfo,
      context_->service_name,
      request.request_id,
      "incoming_request",
      "{\"method\":\"" + request.method + "\",\"path\":\"" + request.path + "\"}");

  if (request.path != "/health" && !auth_middleware_.IsAuthorized(request)) {
    return {.status = 401,
            .body = R"({"error":{"code":"UNAUTHORIZED","message":"missing or invalid bearer token"}})",
            .headers = {{"content-type", "application/json"}, {"x-request-id", request.request_id}}};
  }

  auto response = router_.Handle(request);
  response.headers["x-request-id"] = request.request_id;
  return response;
}

}  // namespace chronos::api::http
