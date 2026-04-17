#pragma once

#include <chrono>
#include <memory>

#include "chronos/persistence/repository.hpp"
#include "chronos/scheduler/executor/local_executor.hpp"
#include "chronos/scheduler/scheduling/duplicate_guard.hpp"

namespace chronos::scheduler::core {

class SchedulerLoop {
 public:
  struct Config {
    std::size_t batch_size{100};
    std::chrono::milliseconds polling_interval{1000};
    std::chrono::seconds drift_guard{30};
  };

  SchedulerLoop(
      Config config,
      std::shared_ptr<persistence::IScheduleRepository> schedule_repository,
      std::shared_ptr<persistence::IExecutionRepository> execution_repository,
      std::shared_ptr<persistence::IOutboxRepository> outbox_repository,
      std::shared_ptr<executor::LocalExecutor> local_executor);

  void Tick();

 private:
  Config config_;
  std::shared_ptr<persistence::IScheduleRepository> schedule_repository_;
  std::shared_ptr<persistence::IExecutionRepository> execution_repository_;
  std::shared_ptr<persistence::IOutboxRepository> outbox_repository_;
  std::shared_ptr<executor::LocalExecutor> local_executor_;
  scheduling::DuplicateDispatchGuard duplicate_guard_;
};

}  // namespace chronos::scheduler::core
