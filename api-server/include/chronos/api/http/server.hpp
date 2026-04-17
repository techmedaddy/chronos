#pragma once

#include <memory>

#include "chronos/api/handlers/context.hpp"
#include "chronos/api/http/router.hpp"
#include "chronos/api/middleware/auth_middleware.hpp"

namespace chronos::api::http {

class ApiServer {
 public:
  ApiServer(
      std::shared_ptr<handlers::HandlerContext> context,
      std::string bearer_token);

  HttpResponse Handle(HttpRequest request) const;

 private:
  Router router_;
  std::shared_ptr<handlers::HandlerContext> context_;
  middleware::AuthMiddleware auth_middleware_;
};

}  // namespace chronos::api::http
