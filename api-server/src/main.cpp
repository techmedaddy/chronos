#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>

#include "chronos/api/application/integration_idempotency_repository.hpp"
#include "chronos/api/handlers/context.hpp"
#include "chronos/api/http/server.hpp"
#include "chronos/coordination/redis_coordination.hpp"
#include "chronos/persistence/in_memory/in_memory_repositories.hpp"

namespace {

volatile std::sig_atomic_t g_running = 1;

void HandleSignal(int) {
  g_running = 0;
}

std::string ToLower(std::string v) {
  std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return v;
}

chronos::api::http::HttpRequest ParseHttpRequest(const std::string& raw) {
  chronos::api::http::HttpRequest req;

  std::istringstream ss(raw);
  std::string line;

  if (std::getline(ss, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    std::istringstream start(line);
    start >> req.method >> req.path;
  }

  while (std::getline(ss, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (line.empty()) {
      break;
    }

    const auto pos = line.find(':');
    if (pos == std::string::npos) {
      continue;
    }

    auto key = ToLower(line.substr(0, pos));
    auto value = line.substr(pos + 1);
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
      value.erase(value.begin());
    }
    req.headers[key] = value;
  }

  std::string body;
  std::getline(ss, body, '\0');
  req.body = body;

  return req;
}

std::optional<std::size_t> ParseContentLengthFromHeaders(const std::string& raw) {
  const auto header_end = raw.find("\r\n\r\n");
  if (header_end == std::string::npos) {
    return std::nullopt;
  }

  std::istringstream ss(raw.substr(0, header_end));
  std::string line;

  // skip request line
  std::getline(ss, line);

  while (std::getline(ss, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    if (line.empty()) {
      break;
    }

    const auto pos = line.find(':');
    if (pos == std::string::npos) {
      continue;
    }

    auto key = ToLower(line.substr(0, pos));
    auto value = line.substr(pos + 1);
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
      value.erase(value.begin());
    }

    if (key == "content-length") {
      try {
        return static_cast<std::size_t>(std::stoul(value));
      } catch (...) {
        return std::nullopt;
      }
    }
  }

  return static_cast<std::size_t>(0);
}

std::string ReadHttpRequestWire(int conn) {
  std::string raw;
  raw.reserve(16384);

  char chunk[4096];
  std::optional<std::size_t> expected_body_len;

  while (true) {
    const auto n = recv(conn, chunk, sizeof(chunk), 0);
    if (n <= 0) {
      break;
    }

    raw.append(chunk, static_cast<std::size_t>(n));

    if (!expected_body_len.has_value()) {
      expected_body_len = ParseContentLengthFromHeaders(raw);
    }

    if (expected_body_len.has_value()) {
      const auto header_end = raw.find("\r\n\r\n");
      if (header_end != std::string::npos) {
        const auto body_start = header_end + 4;
        const auto body_len = raw.size() > body_start ? raw.size() - body_start : 0;
        if (body_len >= *expected_body_len) {
          break;
        }
      }
    }
  }

  return raw;
}

std::string BuildHttpResponse(const chronos::api::http::HttpResponse& res) {
  std::ostringstream out;
  out << "HTTP/1.1 " << res.status << " ";
  switch (res.status) {
    case 200:
      out << "OK";
      break;
    case 201:
      out << "Created";
      break;
    case 400:
      out << "Bad Request";
      break;
    case 401:
      out << "Unauthorized";
      break;
    case 403:
      out << "Forbidden";
      break;
    case 404:
      out << "Not Found";
      break;
    case 409:
      out << "Conflict";
      break;
    case 429:
      out << "Too Many Requests";
      break;
    case 500:
      out << "Internal Server Error";
      break;
    default:
      out << "Status";
      break;
  }
  out << "\r\n";

  std::unordered_map<std::string, std::string> headers = res.headers;
  if (headers.find("content-type") == headers.end()) {
    headers["content-type"] = "application/json";
  }
  headers["content-length"] = std::to_string(res.body.size());
  headers["connection"] = "close";

  for (const auto& [k, v] : headers) {
    out << k << ": " << v << "\r\n";
  }

  out << "\r\n" << res.body;
  return out.str();
}

}  // namespace

int main() {
  using namespace chronos;

  std::signal(SIGINT, HandleSignal);
  std::signal(SIGTERM, HandleSignal);

  auto metrics = std::make_shared<api::handlers::Metrics>();
  auto audit = std::make_shared<persistence::in_memory::InMemoryAuditRepository>();
  auto jobs = std::make_shared<persistence::in_memory::InMemoryJobRepository>();
  auto schedules = std::make_shared<persistence::in_memory::InMemoryScheduleRepository>();
  auto executions = std::make_shared<persistence::in_memory::InMemoryExecutionRepository>(audit);

  auto context = std::make_shared<api::handlers::HandlerContext>();
  context->job_repository = jobs;
  context->schedule_repository = schedules;
  context->execution_repository = executions;
  context->in_memory_execution_repository = executions;
  context->coordination = std::make_shared<coordination::InMemoryRedisCoordination>();
  context->metrics = metrics;
  context->integration_idempotency_repository =
      std::make_shared<api::application::InMemoryIntegrationIdempotencyRepository>();
  context->event_dedupe_repository =
      std::make_shared<api::application::InMemoryEventDedupeRepository>();

  const char* token_env = std::getenv("CHRONOS_BEARER_TOKEN");
  const std::string token = token_env ? std::string(token_env) : std::string("dev-token");

  api::http::ApiServer server(context, token);

  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    std::cerr << "failed to create socket" << std::endl;
    return 1;
  }

  int opt = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(8080);

  if (bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    std::cerr << "failed to bind 0.0.0.0:8080: " << std::strerror(errno) << std::endl;
    close(listen_fd);
    return 1;
  }

  if (listen(listen_fd, 128) < 0) {
    std::cerr << "failed to listen" << std::endl;
    close(listen_fd);
    return 1;
  }

  std::cout << "chronos-api-server listening on 0.0.0.0:8080" << std::endl;

  while (g_running) {
    sockaddr_in client{};
    socklen_t client_len = sizeof(client);
    int conn = accept(listen_fd, reinterpret_cast<sockaddr*>(&client), &client_len);
    if (conn < 0) {
      if (!g_running) {
        break;
      }
      continue;
    }

    const auto raw = ReadHttpRequestWire(conn);
    if (raw.empty()) {
      close(conn);
      continue;
    }

    auto req = ParseHttpRequest(raw);
    if (req.method.empty()) {
      req.method = "GET";
      req.path = "/";
    }

    const auto res = server.Handle(req);
    const auto wire = BuildHttpResponse(res);
    send(conn, wire.data(), wire.size(), 0);
    close(conn);
  }

  close(listen_fd);
  std::cout << "chronos-api-server shutting down" << std::endl;
  return 0;
}
