#include "chronos/scheduler/executor/local_executor.hpp"

#include <optional>

namespace chronos::scheduler::executor {

LocalExecutor::LocalExecutor(
    std::shared_ptr<persistence::IExecutionRepository> execution_repository)
    : execution_repository_(std::move(execution_repository)) {}

bool LocalExecutor::Execute(const std::string& execution_id) {
  if (!execution_repository_->TransitionExecutionState(
          execution_id,
          domain::ExecutionState::kDispatched,
          domain::ExecutionState::kRunning,
          std::nullopt,
          std::nullopt,
          std::string("local-executor"))) {
    return false;
  }

  // Placeholder: task execution runtime would run here.

  return execution_repository_->TransitionExecutionState(
      execution_id,
      domain::ExecutionState::kRunning,
      domain::ExecutionState::kSucceeded,
      std::nullopt,
      std::nullopt,
      std::string("local-executor"));
}

}  // namespace chronos::scheduler::executor
