set(CHRONOS_SCHEDULER_SOURCES
  src/core/scheduler_loop.cpp
  src/core/scheduler_runtime.cpp
  src/scheduling/schedule_calculator.cpp
  src/scheduling/misfire_policy.cpp
  src/scheduling/duplicate_guard.cpp
  src/executor/local_executor.cpp
  src/messaging/rabbitmq_publisher.cpp
  src/reconciliation/reconciliation_job.cpp
  src/leader/lease_election.cpp
  src/leader/fencing_guard.cpp
  src/metrics/metrics.cpp
  src/observability/logger.cpp
)

add_library(chronos-scheduler-lib STATIC ${CHRONOS_SCHEDULER_SOURCES})

target_include_directories(chronos-scheduler-lib
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(chronos-scheduler-lib
  PUBLIC
    chronos-shared-core
)

target_compile_features(chronos-scheduler-lib PUBLIC cxx_std_20)

add_executable(chronos-scheduler src/main.cpp)
target_link_libraries(chronos-scheduler PRIVATE chronos-scheduler-lib)

add_executable(chronos-leader-failover-tests ../tests/leader-failover/leader_failover_tests.cpp)
target_link_libraries(chronos-leader-failover-tests PRIVATE chronos-scheduler-lib)
