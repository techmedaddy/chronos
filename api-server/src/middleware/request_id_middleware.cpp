#include "chronos/api/middleware/request_id_middleware.hpp"

#include <atomic>

namespace chronos::api::middleware {

std::string RequestIdMiddleware::ResolveOrGenerate(const http::HttpRequest& request) {
  const auto it = request.headers.find("x-request-id");
  if (it != request.headers.end() && !it->second.empty()) {
    return it->second;
  }

  static std::atomic<unsigned long long> counter{1};
  return "req-" + std::to_string(counter.fetch_add(1));
}

}  // namespace chronos::api::middleware
