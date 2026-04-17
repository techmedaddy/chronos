#include "chronos/scheduler/leader/lease_election.hpp"

#include <utility>

namespace chronos::scheduler::leader {

std::optional<LeaseRecord> InMemoryLeaseStore::Read(const std::string& lease_key) {
  (void)lease_key;
  std::lock_guard<std::mutex> lock(mu_);
  if (!record_.has_value()) {
    return std::nullopt;
  }

  if (record_->expires_at <= std::chrono::steady_clock::now()) {
    record_.reset();
    return std::nullopt;
  }

  return record_;
}

bool InMemoryLeaseStore::TryAcquire(
    const std::string& lease_key,
    const std::string& candidate_id,
    std::chrono::seconds ttl,
    LeaseRecord* out_record) {
  (void)lease_key;
  std::lock_guard<std::mutex> lock(mu_);
  const auto now = std::chrono::steady_clock::now();

  if (record_.has_value() && record_->expires_at > now) {
    return false;
  }

  LeaseRecord next;
  next.leader_id = candidate_id;
  next.fence_token = "fence-" + std::to_string(token_counter_++);
  next.expires_at = now + ttl;
  record_ = next;

  if (out_record) {
    *out_record = next;
  }

  return true;
}

bool InMemoryLeaseStore::Renew(
    const std::string& lease_key,
    const std::string& candidate_id,
    const std::string& fence_token,
    std::chrono::seconds ttl,
    LeaseRecord* out_record) {
  (void)lease_key;
  std::lock_guard<std::mutex> lock(mu_);
  const auto now = std::chrono::steady_clock::now();

  if (!record_.has_value()) {
    return false;
  }

  if (record_->leader_id != candidate_id || record_->fence_token != fence_token) {
    return false;
  }

  if (record_->expires_at <= now) {
    record_.reset();
    return false;
  }

  record_->expires_at = now + ttl;
  if (out_record) {
    *out_record = *record_;
  }

  return true;
}

bool InMemoryLeaseStore::Release(
    const std::string& lease_key,
    const std::string& candidate_id,
    const std::string& fence_token) {
  (void)lease_key;
  std::lock_guard<std::mutex> lock(mu_);
  if (!record_.has_value()) {
    return false;
  }

  if (record_->leader_id != candidate_id || record_->fence_token != fence_token) {
    return false;
  }

  record_.reset();
  return true;
}

LeaseElection::LeaseElection(
    Config config,
    std::string scheduler_id,
    std::shared_ptr<ILeaseStore> store)
    : config_(std::move(config)),
      scheduler_id_(std::move(scheduler_id)),
      store_(std::move(store)) {}

bool LeaseElection::Campaign() {
  LeaseRecord record;
  const auto ok = store_->TryAcquire(config_.lease_key, scheduler_id_, config_.ttl, &record);
  if (!ok) {
    is_leader_ = false;
    fence_token_.clear();
    return false;
  }

  is_leader_ = true;
  fence_token_ = record.fence_token;
  return true;
}

bool LeaseElection::KeepAlive() {
  if (!is_leader_) {
    return false;
  }

  LeaseRecord record;
  const auto ok = store_->Renew(
      config_.lease_key,
      scheduler_id_,
      fence_token_,
      config_.ttl,
      &record);

  if (!ok) {
    is_leader_ = false;
    fence_token_.clear();
    return false;
  }

  fence_token_ = record.fence_token;
  return true;
}

void LeaseElection::StepDown() {
  if (is_leader_) {
    store_->Release(config_.lease_key, scheduler_id_, fence_token_);
  }
  is_leader_ = false;
  fence_token_.clear();
}

bool LeaseElection::IsLeader() const {
  return is_leader_;
}

std::string LeaseElection::FenceToken() const {
  return fence_token_;
}

std::string LeaseElection::SchedulerId() const {
  return scheduler_id_;
}

}  // namespace chronos::scheduler::leader
