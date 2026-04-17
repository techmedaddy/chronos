#include "chronos/api/handlers/job_handlers.hpp"

#include <atomic>

#include "chronos/api/validation/job_validation.hpp"
#include "chronos/time/clock.hpp"

namespace chronos::api::handlers {
namespace {

std::string NextId(const std::string& prefix) {
  static std::atomic<unsigned long long> counter{1};
  return prefix + "-" + std::to_string(counter.fetch_add(1));
}

std::string EscapeJson(const std::string& value) {
  std::string out;
  out.reserve(value.size());
  for (const auto c : value) {
    if (c == '"') {
      out += "\\\"";
    } else if (c == '\\') {
      out += "\\\\";
    } else {
      out.push_back(c);
    }
  }
  return out;
}

}  // namespace

http::HttpResponse HandleCreateJob(
    const http::HttpRequest& request,
    const std::shared_ptr<HandlerContext>& context) {
  context->metrics->requests_total.fetch_add(1);

  const auto validation = validation::ValidateCreateJobPayload(request.body);
  if (!validation.ok) {
    return {.status = 400,
            .body = "{\"error\":{\"code\":\"VALIDATION_ERROR\",\"message\":\"" +
                    EscapeJson(validation.message) + "\"}}",
            .headers = {{"content-type", "application/json"}}};
  }

  domain::Job job;
  job.job_id = NextId("job");
  job.name = "created-via-api";
  job.task_type = "task.generic";
  job.queue_name = "main_queue";
  job.payload_json = request.body;
  job.created_at = time::UtcNow();
  job.updated_at = job.created_at;

  if (!context->job_repository->CreateJob(job)) {
    return {.status = 409,
            .body = R"({"error":{"code":"CONFLICT","message":"job already exists"}})",
            .headers = {{"content-type", "application/json"}}};
  }

  context->metrics->jobs_created_total.fetch_add(1);

  return {.status = 201,
          .body = "{\"job_id\":\"" + job.job_id +
                  "\",\"job_state\":\"ACTIVE\",\"created_at\":\"" +
                  time::ToIso8601(job.created_at) + "\"}",
          .headers = {{"content-type", "application/json"}}};
}

http::HttpResponse HandleGetJobById(
    const http::HttpRequest& request,
    const std::shared_ptr<HandlerContext>& context) {
  context->metrics->requests_total.fetch_add(1);

  const auto it = request.path_params.find("id");
  if (it == request.path_params.end()) {
    return {.status = 400,
            .body = R"({"error":{"code":"VALIDATION_ERROR","message":"id path param missing"}})",
            .headers = {{"content-type", "application/json"}}};
  }

  const auto job = context->job_repository->GetJobById(it->second);
  if (!job.has_value()) {
    return {.status = 404,
            .body = R"({"error":{"code":"NOT_FOUND","message":"job not found"}})",
            .headers = {{"content-type", "application/json"}}};
  }

  return {.status = 200,
          .body = "{\"job_id\":\"" + job->job_id +
                  "\",\"name\":\"" + EscapeJson(job->name) +
                  "\",\"task_type\":\"" + EscapeJson(job->task_type) +
                  "\",\"job_state\":\"ACTIVE\"}",
          .headers = {{"content-type", "application/json"}}};
}

http::HttpResponse HandleGetJobExecutions(
    const http::HttpRequest& request,
    const std::shared_ptr<HandlerContext>& context) {
  context->metrics->requests_total.fetch_add(1);
  context->metrics->execution_queries_total.fetch_add(1);

  const auto it = request.path_params.find("id");
  if (it == request.path_params.end()) {
    return {.status = 400,
            .body = R"({"error":{"code":"VALIDATION_ERROR","message":"id path param missing"}})",
            .headers = {{"content-type", "application/json"}}};
  }

  const auto executions = context->execution_repository->GetExecutionHistoryForJob(it->second, 50, 0);

  std::string body = "{\"items\":[";
  for (std::size_t i = 0; i < executions.size(); ++i) {
    const auto& e = executions[i];
    body += "{\"execution_id\":\"" + e.execution_id + "\",\"execution_number\":" +
            std::to_string(e.execution_number) + "}";
    if (i + 1 < executions.size()) {
      body += ",";
    }
  }
  body += "],\"next_cursor\":null}";

  return {.status = 200, .body = body, .headers = {{"content-type", "application/json"}}};
}

}  // namespace chronos::api::handlers
