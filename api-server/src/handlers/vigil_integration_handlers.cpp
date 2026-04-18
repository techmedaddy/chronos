#include "chronos/api/handlers/vigil_integration_handlers.hpp"

#include <atomic>
#include <vector>

#include "chronos/api/application/integration_idempotency_repository.hpp"
#include "chronos/api/application/vigil_remediation_service.hpp"
#include "chronos/api/dto/vigil_integration_dto.hpp"
#include "chronos/time/clock.hpp"

namespace chronos::api::handlers {
namespace {

std::string NextId(const std::string& prefix) {
  static std::atomic<unsigned long long> counter{1};
  return prefix + "-" + std::to_string(counter.fetch_add(1));
}

std::string GetHeader(
    const http::HttpRequest& request,
    const std::string& key,
    const std::string& fallback = "") {
  const auto it = request.headers.find(key);
  if (it == request.headers.end() || it->second.empty()) {
    return fallback;
  }
  return it->second;
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

http::HttpResponse BuildError(
    int status,
    const std::string& code,
    const std::string& message,
    const std::string& request_id,
    const std::string& correlation_id,
    bool retryable) {
  const auto now = time::ToIso8601(time::UtcNow());
  return {
      .status = status,
      .body =
          "{"
          "\"error\":{"
          "\"code\":\"" + EscapeJson(code) + "\","
          "\"message\":\"" + EscapeJson(message) + "\","
          "\"httpStatus\":" + std::to_string(status) + ","
          "\"retryable\":" + std::string(retryable ? "true" : "false") +
          "},"
          "\"requestId\":\"" + EscapeJson(request_id) + "\"," 
          "\"correlationId\":\"" + EscapeJson(correlation_id) + "\"," 
          "\"timestamp\":\"" + now + "\""
          "}",
      .headers = {
          {"content-type", "application/json"},
          {"x-request-id", request_id},
          {"x-correlation-id", correlation_id},
      },
  };
}

std::optional<http::HttpResponse> ValidateMandatoryHeaders(const http::HttpRequest& request) {
  const auto request_id = GetHeader(request, "x-request-id", request.request_id);
  const auto correlation_id = GetHeader(request, "x-correlation-id", request.request_id);

  static const std::vector<std::string> kRequiredHeaders{
      "x-tenant-id",
      "idempotency-key",
      "x-request-id",
      "x-correlation-id",
      "x-vigil-incident-id",
      "x-vigil-action-id",
  };

  for (const auto& h : kRequiredHeaders) {
    if (GetHeader(request, h).empty()) {
      return BuildError(
          400,
          "CHRONOS_VALIDATION_ERROR",
          "missing required header: " + h,
          request_id,
          correlation_id,
          false);
    }
  }
  return std::nullopt;
}

std::optional<http::HttpResponse> ValidateCreatePayloadShape(const http::HttpRequest& request) {
  const auto request_id = GetHeader(request, "x-request-id", request.request_id);
  const auto correlation_id = GetHeader(request, "x-correlation-id", request.request_id);

  const auto parsed = dto::ParseCreateRemediationJobDto(request.body);
  if (!parsed.ok) {
    return BuildError(
        400,
        "CHRONOS_VALIDATION_ERROR",
        parsed.message,
        request_id,
        correlation_id,
        false);
  }

  // Header/body tenant alignment.
  const auto tenant_header = GetHeader(request, "x-tenant-id");
  if (!tenant_header.empty() && parsed.dto.tenant_id != tenant_header) {
    return BuildError(
        403,
        "CHRONOS_FORBIDDEN_TENANT",
        "tenant mismatch between header and payload",
        request_id,
        correlation_id,
        false);
  }

  return std::nullopt;
}

std::string NormalizedActionStateFromExecutionState(const std::string& execution_state) {
  if (execution_state == "PENDING") return "queued";
  if (execution_state == "DISPATCHED") return "dispatched";
  if (execution_state == "RUNNING") return "in_progress";
  if (execution_state == "RETRY_PENDING") return "retrying";
  if (execution_state == "SUCCEEDED") return "succeeded";
  if (execution_state == "FAILED") return "failed";
  if (execution_state == "DEAD_LETTER") return "dead_lettered";
  if (execution_state == "CANCEL_REQUESTED") return "cancel_requested";
  return "unknown";
}

std::string IncidentImpactFromExecutionState(const std::string& execution_state) {
  if (execution_state == "SUCCEEDED") {
    return "resolved_candidate";
  }
  if (execution_state == "FAILED" || execution_state == "DEAD_LETTER") {
    return "escalated";
  }
  return "open";
}

}  // namespace

http::HttpResponse HandleCreateVigilRemediationJob(
    const http::HttpRequest& request,
    const std::shared_ptr<HandlerContext>& context) {
  context->metrics->requests_total.fetch_add(1);

  if (const auto error = ValidateMandatoryHeaders(request); error.has_value()) {
    return *error;
  }
  if (const auto error = ValidateCreatePayloadShape(request); error.has_value()) {
    return *error;
  }

  const auto tenant_id = GetHeader(request, "x-tenant-id");
  const auto idempotency_key = GetHeader(request, "idempotency-key");
  const auto request_id = GetHeader(request, "x-request-id", request.request_id);
  const auto correlation_id = GetHeader(request, "x-correlation-id", request.request_id);
  const auto endpoint = std::string("POST:/v1/integrations/vigil/remediation-jobs");

  if (!context->integration_idempotency_repository) {
    return BuildError(
        500,
        "CHRONOS_INTERNAL_ERROR",
        "integration idempotency repository is not configured",
        request_id,
        correlation_id,
        true);
  }

  const auto payload_hash = application::ComputeCanonicalPayloadHash(request.body);

  const auto existing = context->integration_idempotency_repository->Get(
      tenant_id,
      endpoint,
      idempotency_key);

  if (existing.has_value()) {
    if (existing->canonical_payload_hash != payload_hash) {
      return BuildError(
          409,
          "CHRONOS_IDEMPOTENCY_MISMATCH",
          "idempotency key reused with different payload",
          request_id,
          correlation_id,
          false);
    }

    return {
        .status = 202,
        .body = existing->response_body,
        .headers = {
            {"content-type", "application/json"},
            {"idempotency-replayed", "true"},
            {"x-tenant-id", tenant_id},
            {"x-request-id", request_id},
            {"x-correlation-id", correlation_id},
        },
    };
  }

  // optimistic placeholder write to avoid race where multiple parallel requests
  // with same idempotency key all execute business path simultaneously.
  const application::IdempotencyRecord in_progress_record{
      .tenant_id = tenant_id,
      .endpoint = endpoint,
      .idempotency_key = idempotency_key,
      .canonical_payload_hash = payload_hash,
      .response_body = "{\"status\":\"in_progress\"}",
  };
  if (!context->integration_idempotency_repository->Put(in_progress_record)) {
    return BuildError(
        500,
        "CHRONOS_INTERNAL_ERROR",
        "failed to persist idempotency record",
        request_id,
        correlation_id,
        true);
  }

  // re-read after placeholder write for deterministic race handling.
  const auto after_put = context->integration_idempotency_repository->Get(
      tenant_id,
      endpoint,
      idempotency_key);
  if (after_put.has_value() && after_put->response_body != "{\"status\":\"in_progress\"}") {
    if (after_put->canonical_payload_hash != payload_hash) {
      return BuildError(
          409,
          "CHRONOS_IDEMPOTENCY_MISMATCH",
          "idempotency key reused with different payload",
          request_id,
          correlation_id,
          false);
    }
    return {
        .status = 202,
        .body = after_put->response_body,
        .headers = {
            {"content-type", "application/json"},
            {"idempotency-replayed", "true"},
            {"x-tenant-id", tenant_id},
            {"x-request-id", request_id},
            {"x-correlation-id", correlation_id},
        },
    };
  }

  const auto parsed = dto::ParseCreateRemediationJobDto(request.body);
  if (!parsed.ok) {
    return BuildError(
        400,
        "CHRONOS_VALIDATION_ERROR",
        parsed.message,
        request_id,
        correlation_id,
        false);
  }

  application::VigilRemediationService service(context);
  const auto created = service.CreateRemediationJob(
      parsed.dto,
      tenant_id,
      GetHeader(request, "x-vigil-incident-id"),
      GetHeader(request, "x-vigil-action-id"),
      request_id,
      correlation_id,
      idempotency_key);
  if (!created.ok) {
    return BuildError(
        500,
        "CHRONOS_INTERNAL_ERROR",
        created.message,
        request_id,
        correlation_id,
        true);
  }

  const auto now = time::ToIso8601(time::UtcNow());

  const auto response_body =
      "{"
      "\"remediationJobId\":\"" + created.remediation_job_id + "\","
      "\"chronosJobId\":\"" + created.chronos_job_id + "\","
      "\"executionId\":\"" + created.execution_id + "\","
      "\"tenantId\":\"" + tenant_id + "\","
      "\"status\":\"accepted\","
      "\"state\":\"" + created.state + "\","
      "\"createdAt\":\"" + now + "\","
      "\"requestId\":\"" + request_id + "\","
      "\"correlationId\":\"" + correlation_id + "\","
      "\"links\":{"
      "\"job\":\"/v1/jobs/" + created.chronos_job_id + "\","
      "\"execution\":\"/v1/jobs/" + created.chronos_job_id + "/executions/" + created.execution_id + "\","
      "\"integration\":\"/v1/integrations/vigil/remediation-jobs/" + created.remediation_job_id + "\""
      "}"
      "}";

  const application::IdempotencyRecord record{
      .tenant_id = tenant_id,
      .endpoint = endpoint,
      .idempotency_key = idempotency_key,
      .canonical_payload_hash = payload_hash,
      .response_body = response_body,
  };

  if (!context->integration_idempotency_repository->Put(record)) {
    return BuildError(
        500,
        "CHRONOS_INTERNAL_ERROR",
        "failed to persist idempotency record",
        request_id,
        correlation_id,
        true);
  }

  return {
      .status = 202,
      .body = response_body,
      .headers = {
          {"content-type", "application/json"},
          {"idempotency-replayed", "false"},
          {"x-tenant-id", tenant_id},
          {"x-request-id", request_id},
          {"x-correlation-id", correlation_id},
      },
  };
}

http::HttpResponse HandleGetVigilRemediationJobById(
    const http::HttpRequest& request,
    const std::shared_ptr<HandlerContext>& context) {
  context->metrics->requests_total.fetch_add(1);

  if (const auto error = ValidateMandatoryHeaders(request); error.has_value()) {
    return *error;
  }

  const auto tenant_id = GetHeader(request, "x-tenant-id");
  const auto request_id = GetHeader(request, "x-request-id", request.request_id);
  const auto correlation_id = GetHeader(request, "x-correlation-id", request.request_id);

  const auto it = request.path_params.find("id");
  if (it == request.path_params.end() || it->second.empty()) {
    return BuildError(
        400,
        "CHRONOS_VALIDATION_ERROR",
        "id path param missing",
        request_id,
        correlation_id,
        false);
  }

  application::VigilRemediationService service(context);
  const auto lookup = service.GetRemediationJobById(it->second, tenant_id);
  if (!lookup.ok) {
    return BuildError(
        404,
        "CHRONOS_NOT_FOUND",
        lookup.message,
        request_id,
        correlation_id,
        false);
  }

  const auto body =
      "{"
      "\"remediationJobId\":\"" + lookup.record.remediation_job_id + "\","
      "\"tenantId\":\"" + lookup.record.tenant_id + "\","
      "\"status\":\"accepted\","
      "\"state\":\"scheduled\","
      "\"executionState\":\"" + lookup.record.execution_state + "\","
      "\"vigilActionState\":\"" + NormalizedActionStateFromExecutionState(lookup.record.execution_state) + "\","
      "\"incidentImpact\":\"" + IncidentImpactFromExecutionState(lookup.record.execution_state) + "\","
      "\"vigilIncidentId\":\"" + lookup.record.vigil_incident_id + "\","
      "\"vigilActionId\":\"" + lookup.record.vigil_action_id + "\","
      "\"requestId\":\"" + lookup.record.request_id + "\","
      "\"correlationId\":\"" + lookup.record.correlation_id + "\","
      "\"chronosJobId\":\"" + lookup.record.chronos_job_id + "\","
      "\"executionId\":\"" + lookup.record.execution_id + "\""
      "}";

  return {
      .status = 200,
      .body = body,
      .headers = {
          {"content-type", "application/json"},
          {"x-request-id", request_id},
          {"x-correlation-id", correlation_id},
      },
  };
}

http::HttpResponse HandleCancelVigilRemediationJob(
    const http::HttpRequest& request,
    const std::shared_ptr<HandlerContext>& context) {
  context->metrics->requests_total.fetch_add(1);

  if (const auto error = ValidateMandatoryHeaders(request); error.has_value()) {
    return *error;
  }

  const auto tenant_id = GetHeader(request, "x-tenant-id");
  const auto request_id = GetHeader(request, "x-request-id", request.request_id);
  const auto correlation_id = GetHeader(request, "x-correlation-id", request.request_id);

  constexpr const char* kPrefix = "/v1/integrations/vigil/remediation-jobs/";
  constexpr const char* kCancelSuffix = ":cancel";

  std::string remediation_job_id;

  if (const auto id_cancel_it = request.path_params.find("id:cancel");
      id_cancel_it != request.path_params.end() && !id_cancel_it->second.empty()) {
    remediation_job_id = id_cancel_it->second;
  } else if (const auto id_it = request.path_params.find("id");
             id_it != request.path_params.end() && !id_it->second.empty()) {
    remediation_job_id = id_it->second;
  }

  if (remediation_job_id.empty()) {
    const auto prefix_pos = request.path.find(kPrefix);
    const auto suffix_pos = request.path.rfind(kCancelSuffix);
    if (prefix_pos != std::string::npos && suffix_pos != std::string::npos && suffix_pos > prefix_pos) {
      const auto start = prefix_pos + std::char_traits<char>::length(kPrefix);
      remediation_job_id = request.path.substr(start, suffix_pos - start);
    }
  }

  const auto suffix_len = std::char_traits<char>::length(kCancelSuffix);
  if (remediation_job_id.size() > suffix_len &&
      remediation_job_id.rfind(kCancelSuffix) == remediation_job_id.size() - suffix_len) {
    remediation_job_id = remediation_job_id.substr(0, remediation_job_id.size() - suffix_len);
  }

  if (!remediation_job_id.empty()) {
    const auto extra_colon = remediation_job_id.find(':');
    if (extra_colon != std::string::npos) {
      remediation_job_id = remediation_job_id.substr(0, extra_colon);
    }
  }

  if (remediation_job_id.empty()) {
    return BuildError(
        400,
        "CHRONOS_VALIDATION_ERROR",
        "id path param missing",
        request_id,
        correlation_id,
        false);
  }

  application::VigilRemediationService service(context);
  auto cancelled = service.CancelRemediationJob(
      remediation_job_id,
      tenant_id,
      request_id,
      correlation_id);

  // Fallback path: if remediation id lookup fails, resolve by vigil action id index.
  if (!cancelled.ok && cancelled.message == "remediation job not found") {
    const auto action_id = GetHeader(request, "x-vigil-action-id");
    if (!action_id.empty()) {
      const auto by_action = service.GetByVigilActionId(action_id, tenant_id);
      if (by_action.ok) {
        cancelled = service.CancelRemediationJob(
            by_action.record.remediation_job_id,
            tenant_id,
            request_id,
            correlation_id);
      }
    }
  }

  if (!cancelled.ok) {
    return BuildError(
        409,
        "CHRONOS_STATE_CONFLICT",
        cancelled.message,
        request_id,
        correlation_id,
        false);
  }

  const auto body =
      "{"
      "\"remediationJobId\":\"" + cancelled.remediation_job_id + "\","
      "\"tenantId\":\"" + cancelled.tenant_id + "\","
      "\"status\":\"" + std::string(cancelled.cancelled ? "cancelled" : "cancel_requested") + "\","
      "\"executionState\":\"" + cancelled.resulting_execution_state + "\""
      "}";

  return {
      .status = 202,
      .body = body,
      .headers = {
          {"content-type", "application/json"},
          {"x-request-id", request_id},
          {"x-correlation-id", correlation_id},
      },
  };
}

http::HttpResponse HandleGetVigilActionStatus(
    const http::HttpRequest& request,
    const std::shared_ptr<HandlerContext>& context) {
  context->metrics->requests_total.fetch_add(1);

  if (const auto error = ValidateMandatoryHeaders(request); error.has_value()) {
    return *error;
  }

  const auto tenant_id = GetHeader(request, "x-tenant-id");
  const auto request_id = GetHeader(request, "x-request-id", request.request_id);
  const auto correlation_id = GetHeader(request, "x-correlation-id", request.request_id);

  const auto it = request.path_params.find("vigilActionId");
  if (it == request.path_params.end() || it->second.empty()) {
    return BuildError(
        400,
        "CHRONOS_VALIDATION_ERROR",
        "vigilActionId path param missing",
        request_id,
        correlation_id,
        false);
  }

  application::VigilRemediationService service(context);
  const auto lookup = service.GetByVigilActionId(it->second, tenant_id);
  if (!lookup.ok) {
    return BuildError(
        404,
        "CHRONOS_NOT_FOUND",
        lookup.message,
        request_id,
        correlation_id,
        false);
  }

  const auto body =
      "{"
      "\"vigilActionId\":\"" + lookup.record.vigil_action_id + "\","
      "\"tenantId\":\"" + lookup.record.tenant_id + "\","
      "\"executionState\":\"" + lookup.record.execution_state + "\","
      "\"status\":\"" + NormalizedActionStateFromExecutionState(lookup.record.execution_state) + "\","
      "\"incidentImpact\":\"" + IncidentImpactFromExecutionState(lookup.record.execution_state) + "\","
      "\"remediationJobId\":\"" + lookup.record.remediation_job_id + "\","
      "\"chronosJobId\":\"" + lookup.record.chronos_job_id + "\","
      "\"executionId\":\"" + lookup.record.execution_id + "\""
      "}";

  return {
      .status = 200,
      .body = body,
      .headers = {
          {"content-type", "application/json"},
          {"x-request-id", request_id},
          {"x-correlation-id", correlation_id},
      },
  };
}

}  // namespace chronos::api::handlers
