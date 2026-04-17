#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "chronos/api/http/types.hpp"

namespace chronos::api::http {

class Router {
 public:
  void Register(const std::string& method, const std::string& path_pattern, Handler handler);
  HttpResponse Handle(const HttpRequest& request) const;

 private:
  struct Route {
    std::string method;
    std::string path_pattern;
    Handler handler;
  };

  std::vector<Route> routes_;

  static bool MatchAndExtract(
      const std::string& pattern,
      const std::string& path,
      std::unordered_map<std::string, std::string>* out_params);
};

}  // namespace chronos::api::http
