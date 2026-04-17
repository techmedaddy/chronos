set(CHRONOS_API_SERVER_SOURCES
  src/http/router.cpp
  src/http/server.cpp
  src/middleware/auth_middleware.cpp
  src/middleware/request_id_middleware.cpp
  src/validation/job_validation.cpp
  src/handlers/job_handlers.cpp
  src/handlers/schedule_handlers.cpp
  src/handlers/health_handlers.cpp
  src/handlers/metrics_handlers.cpp
  src/observability/logger.cpp
)

add_library(chronos-api-server-lib STATIC ${CHRONOS_API_SERVER_SOURCES})

target_include_directories(chronos-api-server-lib
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(chronos-api-server-lib
  PUBLIC
    chronos-shared-core
)

target_compile_features(chronos-api-server-lib PUBLIC cxx_std_20)

add_executable(chronos-api-server src/main.cpp)
target_link_libraries(chronos-api-server PRIVATE chronos-api-server-lib)
