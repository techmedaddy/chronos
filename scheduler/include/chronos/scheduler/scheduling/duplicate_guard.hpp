#pragma once

#include <mutex>
#include <string>
#include <unordered_set>

namespace chronos::scheduler::scheduling {

class DuplicateDispatchGuard {
 public:
  bool TryMarkDispatched(const std::string& dispatch_key);
  void Clear(const std::string& dispatch_key);

 private:
  std::mutex mu_;
  std::unordered_set<std::string> dispatched_;
};

}  // namespace chronos::scheduler::scheduling
