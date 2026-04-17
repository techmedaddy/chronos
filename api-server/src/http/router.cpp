#include "chronos/api/http/router.hpp"

#include <sstream>
#include <utility>

namespace chronos::api::http {
namespace {

std::vector<std::string> SplitPath(const std::string& path) {
  std::vector<std::string> parts;
  std::stringstream ss(path);
  std::string part;
  while (std::getline(ss, part, '/')) {
    if (!part.empty()) {
      parts.push_back(part);
    }
  }
  return parts;
}

}  // namespace

void Router::Register(const std::string& method, const std::string& path_pattern, Handler handler) {
  routes_.push_back(Route{.method = method, .path_pattern = path_pattern, .handler = std::move(handler)});
}

HttpResponse Router::Handle(const HttpRequest& request) const {
  for (const auto& route : routes_) {
    if (route.method != request.method) {
      continue;
    }

    std::unordered_map<std::string, std::string> params;
    if (!MatchAndExtract(route.path_pattern, request.path, &params)) {
      continue;
    }

    auto resolved = request;
    resolved.path_params = std::move(params);
    return route.handler(resolved);
  }

  return HttpResponse{.status = 404,
                      .body = R"({"error":{"code":"NOT_FOUND","message":"route not found"}})",
                      .headers = {{"content-type", "application/json"}}};
}

bool Router::MatchAndExtract(
    const std::string& pattern,
    const std::string& path,
    std::unordered_map<std::string, std::string>* out_params) {
  const auto pattern_parts = SplitPath(pattern);
  const auto path_parts = SplitPath(path);

  if (pattern_parts.size() != path_parts.size()) {
    return false;
  }

  for (std::size_t i = 0; i < pattern_parts.size(); ++i) {
    const auto& p = pattern_parts[i];
    const auto& v = path_parts[i];

    if (!p.empty() && p[0] == ':') {
      (*out_params)[p.substr(1)] = v;
      continue;
    }

    if (p != v) {
      return false;
    }
  }

  return true;
}

}  // namespace chronos::api::http
