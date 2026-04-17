#pragma once

#include <memory>
#include <string>

#include "chronos/persistence/repository.hpp"

namespace chronos::scheduler::executor {

class LocalExecutor {
 public:
  explicit LocalExecutor(std::shared_ptr<persistence::IExecutionRepository> execution_repository);

  bool Execute(const std::string& execution_id);

 private:
  std::shared_ptr<persistence::IExecutionRepository> execution_repository_;
};

}  // namespace chronos::scheduler::executor
