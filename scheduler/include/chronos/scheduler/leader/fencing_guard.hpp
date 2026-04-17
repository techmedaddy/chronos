#pragma once

#include <string>

namespace chronos::scheduler::leader {

class FencingGuard {
 public:
  FencingGuard(std::string scheduler_id, std::string fence_token);

  bool Allows(const std::string& active_scheduler_id, const std::string& active_fence_token) const;
  [[nodiscard]] const std::string& scheduler_id() const;
  [[nodiscard]] const std::string& fence_token() const;

 private:
  std::string scheduler_id_;
  std::string fence_token_;
};

}  // namespace chronos::scheduler::leader
