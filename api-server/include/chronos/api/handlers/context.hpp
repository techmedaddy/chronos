#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "chronos/persistence/repository.hpp"

namespace chronos::api::handlers {

struct Metrics {
  std::atomic<unsigned long long> requests_total{0};
  std::atomic<unsigned long long> jobs_created_total{0};
  std::atomic<unsigned long long> schedules_created_total{0};
  std::atomic<unsigned long long> execution_queries_total{0};
};

struct HandlerContext {
  std::shared_ptr<persistence::IJobRepository> job_repository;
  std::shared_ptr<persistence::IScheduleRepository> schedule_repository;
  std::shared_ptr<persistence::IExecutionRepository> execution_repository;
  std::shared_ptr<Metrics> metrics;
  std::string service_name{"api-server"};
};

}  // namespace chronos::api::handlers
