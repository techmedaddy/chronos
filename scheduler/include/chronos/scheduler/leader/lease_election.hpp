#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace chronos::scheduler::leader {

struct LeaseRecord {
  std::string leader_id;
  std::string fence_token;
  std::chrono::steady_clock::time_point expires_at;
};

class ILeaseStore {
 public:
  virtual ~ILeaseStore() = default;

  virtual std::optional<LeaseRecord> Read(const std::string& lease_key) = 0;
  virtual bool TryAcquire(
      const std::string& lease_key,
      const std::string& candidate_id,
      std::chrono::seconds ttl,
      LeaseRecord* out_record) = 0;
  virtual bool Renew(
      const std::string& lease_key,
      const std::string& candidate_id,
      const std::string& fence_token,
      std::chrono::seconds ttl,
      LeaseRecord* out_record) = 0;
  virtual bool Release(
      const std::string& lease_key,
      const std::string& candidate_id,
      const std::string& fence_token) = 0;
};

class InMemoryLeaseStore final : public ILeaseStore {
 public:
  std::optional<LeaseRecord> Read(const std::string& lease_key) override;
  bool TryAcquire(
      const std::string& lease_key,
      const std::string& candidate_id,
      std::chrono::seconds ttl,
      LeaseRecord* out_record) override;
  bool Renew(
      const std::string& lease_key,
      const std::string& candidate_id,
      const std::string& fence_token,
      std::chrono::seconds ttl,
      LeaseRecord* out_record) override;
  bool Release(
      const std::string& lease_key,
      const std::string& candidate_id,
      const std::string& fence_token) override;

 private:
  std::mutex mu_;
  std::optional<LeaseRecord> record_;
  unsigned long long token_counter_{1};
};

class LeaseElection {
 public:
  struct Config {
    std::string lease_key{"chronos/scheduler/leader"};
    std::chrono::seconds ttl{10};
  };

  LeaseElection(
      Config config,
      std::string scheduler_id,
      std::shared_ptr<ILeaseStore> store);

  bool Campaign();
  bool KeepAlive();
  void StepDown();

  [[nodiscard]] bool IsLeader() const;
  [[nodiscard]] std::string FenceToken() const;
  [[nodiscard]] std::string SchedulerId() const;

 private:
  Config config_;
  std::string scheduler_id_;
  std::shared_ptr<ILeaseStore> store_;
  bool is_leader_{false};
  std::string fence_token_;
};

}  // namespace chronos::scheduler::leader
