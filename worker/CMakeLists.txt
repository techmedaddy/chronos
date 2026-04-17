set(CHRONOS_WORKER_SOURCES
  src/runtime/thread_pool.cpp
  src/runtime/worker_runtime.cpp
  src/task/task_contract.cpp
  src/task/task_registry.cpp
  src/execution/claim_manager.cpp
  src/execution/heartbeat_manager.cpp
  src/execution/result_handler.cpp
  src/messaging/rabbitmq_consumer.cpp
  src/observability/logger.cpp
)

add_library(chronos-worker-lib STATIC ${CHRONOS_WORKER_SOURCES})

target_include_directories(chronos-worker-lib
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(chronos-worker-lib
  PUBLIC
    chronos-shared-core
)

target_compile_features(chronos-worker-lib PUBLIC cxx_std_20)

add_executable(chronos-worker src/main.cpp)
target_link_libraries(chronos-worker PRIVATE chronos-worker-lib)

add_executable(chronos-worker-stress src/stress/local_stress.cpp)
target_link_libraries(chronos-worker-stress PRIVATE chronos-worker-lib)
