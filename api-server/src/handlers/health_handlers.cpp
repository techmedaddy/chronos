#include "chronos/api/handlers/health_handlers.hpp"

namespace chronos::api::handlers {

http::HttpResponse HandleHealth(const http::HttpRequest& request) {
  (void)request;
  return {.status = 200,
          .body = R"({"status":"ok","service":"api-server"})",
          .headers = {{"content-type", "application/json"}}};
}

}  // namespace chronos::api::handlers
