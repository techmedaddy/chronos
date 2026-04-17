#pragma once

#include <chrono>
#include <memory>

#include "chronos/messaging/queue_broker.hpp"
#include "chronos/persistence/repository.hpp"
#include "chronos/scheduler/executor/local_executor.hpp"
#include "chronos/scheduler/leader/lease_election.hpp"
#include "chronos/scheduler/messaging/rabbitmq_publisher.hpp"
#include "chronos/scheduler/scheduling/duplicate_guard.hpp"

namespace chronos::scheduler::core {

class SchedulerLoop {
 public:
  struct Config {
    std::size_t batch_size{100};
    std::chrono::milliseconds polling_interval{1000};
    std::chrono::seconds drift_guard{30};
    std::string lease_key{"chronos/scheduler/leader"};
  };

  SchedulerLoop(
      Config config,
      std::shared_ptr<persistence::IScheduleRepository> schedule_repository,
      std::shared_ptr<persistence::IExecutionRepository> execution_repository,
      std::shared_ptr<persistence::IOutboxRepository> outbox_repository,
      std::shared_ptr<executor::LocalExecutor> local_executor,
      std::shared_ptr<messaging::RabbitMqPublisher> publisher,
      std::shared_ptr<chronos::messaging::IQueueBroker> broker,
      std::shared_ptr<leader::ILeaseStore> lease_store);

  // Returns false when fencing check fails.
  bool Tick(const std::string& scheduler_id, const std::string& fence_token);

 private:
  Config config_;
  std::shared_ptr<persistence::IScheduleRepository> schedule_repository_;
  std::shared_ptr<persistence::IExecutionRepository> execution_repository_;
  std::shared_ptr<persistence::IOutboxRepository> outbox_repository_;
  std::shared_ptr<executor::LocalExecutor> local_executor_;
  std::shared_ptr<messaging::RabbitMqPublisher> publisher_;
  std::shared_ptr<chronos::messaging::IQueueBroker> broker_;
  std::shared_ptr<leader::ILeaseStore> lease_store_;
  scheduling::DuplicateDispatchGuard duplicate_guard_;
};

}  // namespace chronos::scheduler::core
