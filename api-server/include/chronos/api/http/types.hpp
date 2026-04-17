#pragma once

#include <functional>
#include <string>
#include <unordered_map>

namespace chronos::api::http {

struct HttpRequest {
  std::string method;
  std::string path;
  std::string body;
  std::unordered_map<std::string, std::string> headers;
  std::unordered_map<std::string, std::string> path_params;
  std::string request_id;
};

struct HttpResponse {
  int status{200};
  std::string body;
  std::unordered_map<std::string, std::string> headers;
};

using Handler = std::function<HttpResponse(const HttpRequest&)>;

}  // namespace chronos::api::http
