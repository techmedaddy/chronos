# scheduler

The scheduler is the orchestration component that detects due work and dispatches
execution attempts.

It is the only component that decides when a job should run.

## Phase 6 status

Added distributed-scheduler runtime primitives:

- lease-based leader election abstraction (local in-memory lease store)
- leader-only dispatch loop, follower warm mode
- fencing-token checks before scheduling
- bounded, jittered scan runtime with catch-up caps
- scheduler election/scan/dispatch metrics
- leader failover test harness

Current local mode is implementation-complete at architecture level; production etcd adapter wiring is the next hardening step.
