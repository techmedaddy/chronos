#pragma once

#include <string>

#include "chronos/api/http/types.hpp"

namespace chronos::api::middleware {

class RequestIdMiddleware {
 public:
  static std::string ResolveOrGenerate(const http::HttpRequest& request);
};

}  // namespace chronos::api::middleware
