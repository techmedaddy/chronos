#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include "chronos/messaging/in_memory_queue_broker.hpp"
#include "chronos/messaging/message_codec.hpp"
#include "chronos/persistence/in_memory/in_memory_repositories.hpp"
#include "chronos/time/clock.hpp"
#include "chronos/worker/messaging/rabbitmq_consumer.hpp"
#include "chronos/worker/runtime/worker_runtime.hpp"
#include "chronos/worker/task/task_registry.hpp"

int main() {
  using namespace chronos;

  auto audit = std::make_shared<persistence::in_memory::InMemoryAuditRepository>();
  auto executions = std::make_shared<persistence::in_memory::InMemoryExecutionRepository>(audit);

  auto registry = std::make_shared<worker::task::TaskRegistry>();
  registry->Register("sample.echo", [](const worker::task::TaskInput& input,
                                        const worker::task::CancellationToken&) {
    worker::task::TaskResult result;
    result.kind = worker::task::TaskResultKind::kSuccess;
    result.metadata_json = "{\"echo\":" + input.payload_json + "}";
    return result;
  });

  domain::JobExecution execution;
  execution.execution_id = "execution-demo-1";
  execution.job_id = "job-demo-1";
  execution.execution_number = 1;
  execution.scheduled_at = time::UtcNow();
  execution.state = domain::ExecutionState::kDispatched;
  execution.max_attempts = 3;
  execution.created_at = time::UtcNow();
  execution.updated_at = execution.created_at;
  executions->CreateExecution(execution);

  worker::runtime::WorkerRuntime::Config config;
  config.worker_id = "worker-local-1";
  config.concurrency = 4;

  auto runtime = std::make_shared<worker::runtime::WorkerRuntime>(config, executions, registry);
  auto broker = std::make_shared<messaging::InMemoryQueueBroker>();
  worker::messaging::RabbitMqConsumer consumer(broker, runtime, config.worker_id);

  messaging::ExecutionDispatchMessage message;
  message.trace_id = "trace-demo-1";
  message.job_id = "job-demo-1";
  message.execution_id = "execution-demo-1";
  message.attempt = 1;
  message.scheduled_at = time::UtcNow();
  message.idempotency_key = "idem-1";
  message.payload_json = "{\"message\":\"hello\"}";

  broker->Publish(messaging::kMainQueue, messaging::EncodeDispatchMessage(message), true);
  consumer.ConsumeOnceFromMain();

  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  runtime->Shutdown();

  std::cout << "chronos-worker completed demo execution via main_queue consumer" << std::endl;
  return 0;
}
