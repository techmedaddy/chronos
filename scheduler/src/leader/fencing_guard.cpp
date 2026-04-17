#include "chronos/scheduler/leader/fencing_guard.hpp"

#include <utility>

namespace chronos::scheduler::leader {

FencingGuard::FencingGuard(std::string scheduler_id, std::string fence_token)
    : scheduler_id_(std::move(scheduler_id)),
      fence_token_(std::move(fence_token)) {}

bool FencingGuard::Allows(
    const std::string& active_scheduler_id,
    const std::string& active_fence_token) const {
  return scheduler_id_ == active_scheduler_id && fence_token_ == active_fence_token;
}

const std::string& FencingGuard::scheduler_id() const {
  return scheduler_id_;
}

const std::string& FencingGuard::fence_token() const {
  return fence_token_;
}

}  // namespace chronos::scheduler::leader
