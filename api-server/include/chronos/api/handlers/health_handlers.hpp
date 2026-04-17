#pragma once

#include "chronos/api/http/types.hpp"

namespace chronos::api::handlers {

http::HttpResponse HandleHealth(const http::HttpRequest& request);

}  // namespace chronos::api::handlers
