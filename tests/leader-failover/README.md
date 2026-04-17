# Leader Failover Tests

Covers distributed scheduler lease and failover behavior.

Current checks:

- lease-expiry leader handoff between scheduler instances
- fencing-token invalidation for old leader after lease loss

Executable target: `chronos-leader-failover-tests` (defined in `scheduler/CMakeLists.txt`).
