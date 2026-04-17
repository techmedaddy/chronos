#include "chronos/api/middleware/auth_middleware.hpp"

#include <utility>

namespace chronos::api::middleware {

AuthMiddleware::AuthMiddleware(std::string expected_bearer_token)
    : expected_bearer_token_(std::move(expected_bearer_token)) {}

bool AuthMiddleware::IsAuthorized(const http::HttpRequest& request) const {
  const auto it = request.headers.find("authorization");
  if (it == request.headers.end()) {
    return false;
  }

  const std::string prefix = "Bearer ";
  if (it->second.rfind(prefix, 0) != 0) {
    return false;
  }

  const auto token = it->second.substr(prefix.size());
  return !expected_bearer_token_.empty() && token == expected_bearer_token_;
}

}  // namespace chronos::api::middleware
