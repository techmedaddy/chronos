#pragma once

#include <memory>

#include "chronos/api/handlers/context.hpp"
#include "chronos/api/http/types.hpp"

namespace chronos::api::handlers {

http::HttpResponse HandleMetrics(
    const http::HttpRequest& request,
    const std::shared_ptr<HandlerContext>& context);

}  // namespace chronos::api::handlers
