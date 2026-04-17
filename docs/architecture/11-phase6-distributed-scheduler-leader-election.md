# Phase 6: Distributed Scheduler and Leader Election (MVP)

This document records Phase 6 implementation status.

## Implemented scope

### 6.1 etcd lease-based leader election (abstraction + local store)

Implemented lease election interfaces and in-memory lease store with TTL behavior:

- acquire lease
- renew lease
- release lease
- read active lease

Files:

- `scheduler/include/chronos/scheduler/leader/lease_election.hpp`
- `scheduler/src/leader/lease_election.cpp`

> Note: in-memory lease store is a local etcd analogue. Production etcd client adapter is next.

### 6.2 Leader-only dispatch loop, followers warm

Implemented scheduler runtime that:

- campaigns when follower
- renews when leader
- runs dispatch scan only when leader
- followers stay warm through jittered loop cycles

Files:

- `scheduler/include/chronos/scheduler/core/scheduler_runtime.hpp`
- `scheduler/src/core/scheduler_runtime.cpp`

### 6.3 Fencing logic

Implemented fencing guard and loop-level lease verification:

- each dispatch tick validates active lease leader id + fence token
- stale leader token is blocked from scheduling

Files:

- `scheduler/include/chronos/scheduler/leader/fencing_guard.hpp`
- `scheduler/src/leader/fencing_guard.cpp`
- `scheduler/src/core/scheduler_loop.cpp`

### 6.4 Efficient scanning controls

Implemented scan efficiency controls in runtime config:

- bounded batch count per runtime tick
- bounded catch-up window
- jittered loop intervals
- metrics update for scan lag and dispatch latency

Files:

- `scheduler/src/core/scheduler_runtime.cpp`

### 6.5 Leader-failover tests

Implemented failover test harness with:

- lease expiry and handoff behavior across two scheduler instances
- fencing token invalidation for old leader

Files:

- `tests/leader-failover/leader_failover_tests.cpp`
- `tests/leader-failover/README.md`
- target: `chronos-leader-failover-tests`

### 6.6 Metrics for election churn, scan lag, dispatch latency

Implemented scheduler metrics struct and prometheus formatter containing:

- election attempts/wins/losses/churn
- scan batches
- dispatch attempts/success
- scan lag ms
- dispatch latency ms

Files:

- `scheduler/include/chronos/scheduler/metrics/metrics.hpp`
- `scheduler/src/metrics/metrics.cpp`
- `scheduler/src/main.cpp` emits metrics snapshot

## Scheduler integration changes

- `SchedulerLoop::Tick` now takes `(scheduler_id, fence_token)` and returns whether dispatch ran.
- loop constructor now receives lease store and enforces lease/fencing checks.
- scheduler main now runs through `SchedulerRuntime` orchestration.

## Validation status

- Source-level syntax checks (`g++ -fsyntax-only`) pass for scheduler and all cross-module dependencies.
- Full CMake build remains blocked until `cmake` is available in PATH.

## Exit criteria status (Phase 6 MVP)

- lease-based leader election: **done (abstraction + in-memory lease store)**
- leader-only dispatch, follower warm loops: **done**
- fencing after lease loss: **done**
- efficient scan controls: **done**
- leader failover tests: **done**
- election/lag/latency metrics: **done**

## Known MVP limitations

1. Real etcd client/lease API adapter still pending (current implementation uses in-memory lease store).
2. Dispatch latency metric currently derived from tick loop timing, not per-message broker round-trip timestamps.
3. Scan window optimization is runtime-bounded but does not yet push SQL time-window predicates into DB queries.
