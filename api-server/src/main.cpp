#include <iostream>
#include <memory>
#include <string>

#include "chronos/api/handlers/context.hpp"
#include "chronos/api/http/server.hpp"
#include "chronos/persistence/in_memory/in_memory_repositories.hpp"

int main() {
  using namespace chronos;

  auto metrics = std::make_shared<api::handlers::Metrics>();
  auto audit = std::make_shared<persistence::in_memory::InMemoryAuditRepository>();
  auto jobs = std::make_shared<persistence::in_memory::InMemoryJobRepository>();
  auto schedules = std::make_shared<persistence::in_memory::InMemoryScheduleRepository>();
  auto executions = std::make_shared<persistence::in_memory::InMemoryExecutionRepository>(audit);

  auto context = std::make_shared<api::handlers::HandlerContext>();
  context->job_repository = jobs;
  context->schedule_repository = schedules;
  context->execution_repository = executions;
  context->metrics = metrics;

  api::http::ApiServer server(context, "dev-token");

  // Placeholder loop for Phase 2 without binding a real TCP HTTP listener.
  // Integration harnesses can instantiate ApiServer and call Handle().
  std::cout << "chronos-api-server initialized (in-process mode)" << std::endl;

  // Demo health request
  api::http::HttpRequest health_req{.method = "GET", .path = "/health"};
  const auto health_res = server.Handle(health_req);
  std::cout << "health response: " << health_res.status << " " << health_res.body << std::endl;

  return 0;
}
