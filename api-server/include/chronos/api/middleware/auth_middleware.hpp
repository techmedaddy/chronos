#pragma once

#include <string>

#include "chronos/api/http/types.hpp"

namespace chronos::api::middleware {

class AuthMiddleware {
 public:
  explicit AuthMiddleware(std::string expected_bearer_token);

  bool IsAuthorized(const http::HttpRequest& request) const;

 private:
  std::string expected_bearer_token_;
};

}  // namespace chronos::api::middleware
