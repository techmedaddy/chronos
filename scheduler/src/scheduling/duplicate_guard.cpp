#include "chronos/scheduler/scheduling/duplicate_guard.hpp"

namespace chronos::scheduler::scheduling {

bool DuplicateDispatchGuard::TryMarkDispatched(const std::string& dispatch_key) {
  std::lock_guard<std::mutex> lock(mu_);
  const auto [_, inserted] = dispatched_.insert(dispatch_key);
  return inserted;
}

void DuplicateDispatchGuard::Clear(const std::string& dispatch_key) {
  std::lock_guard<std::mutex> lock(mu_);
  dispatched_.erase(dispatch_key);
}

}  // namespace chronos::scheduler::scheduling
