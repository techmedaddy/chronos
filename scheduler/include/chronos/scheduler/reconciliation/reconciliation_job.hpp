#pragma once

#include <memory>
#include <string>
#include <vector>

#include "chronos/messaging/queue_broker.hpp"
#include "chronos/persistence/repository.hpp"
#include "chronos/scheduler/messaging/rabbitmq_publisher.hpp"

namespace chronos::scheduler::reconciliation {

class ReconciliationJob {
 public:
  struct DriftRecord {
    std::string execution_id;
    std::string reason;
  };

  ReconciliationJob(
      std::shared_ptr<persistence::IExecutionRepository> execution_repository,
      std::shared_ptr<messaging::RabbitMqPublisher> publisher,
      std::shared_ptr<chronos::messaging::IQueueBroker> broker);

  std::vector<DriftRecord> DetectAndRepair(
      const std::vector<std::string>& expected_inflight_execution_ids,
      const std::string& trace_id);

 private:
  std::shared_ptr<persistence::IExecutionRepository> execution_repository_;
  std::shared_ptr<messaging::RabbitMqPublisher> publisher_;
  std::shared_ptr<chronos::messaging::IQueueBroker> broker_;
};

}  // namespace chronos::scheduler::reconciliation
