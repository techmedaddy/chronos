set(CHRONOS_SHARED_CORE_SOURCES
  src/domain/models.cpp
  src/serialization/enum_codec.cpp
  src/time/clock.cpp
  src/retry/backoff.cpp
  src/state_machine/execution_state_machine.cpp
  src/persistence/postgres/postgres_repositories.cpp
  src/persistence/in_memory/in_memory_repositories.cpp
)

add_library(chronos-shared-core STATIC ${CHRONOS_SHARED_CORE_SOURCES})

target_include_directories(chronos-shared-core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(chronos-shared-core PUBLIC cxx_std_20)

if(CHRONOS_BUILD_TESTS)
  add_executable(chronos-state-machine-tests
    tests/state_machine_tests.cpp
  )
  target_link_libraries(chronos-state-machine-tests PRIVATE chronos-shared-core)
  add_test(NAME chronos-state-machine-tests COMMAND chronos-state-machine-tests)

  add_executable(chronos-backoff-tests
    tests/backoff_tests.cpp
  )
  target_link_libraries(chronos-backoff-tests PRIVATE chronos-shared-core)
  add_test(NAME chronos-backoff-tests COMMAND chronos-backoff-tests)
endif()
