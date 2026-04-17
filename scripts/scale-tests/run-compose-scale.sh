#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT_DIR"

if ! command -v docker >/dev/null 2>&1; then
  echo "docker not found"
  exit 1
fi

COMPOSE="docker compose"

$COMPOSE up -d --build
$COMPOSE up -d --scale worker=10

echo "Waiting for services to settle..."
sleep 10

echo "=== Container status ==="
$COMPOSE ps

echo "=== Queue depth sample (RabbitMQ management API) ==="
curl -s -u guest:guest http://localhost:15672/api/queues | head -c 1000 || true
echo

echo "=== API metrics ==="
curl -s http://localhost:8080/metrics | head -n 50 || true

echo "=== Scheduler metrics ==="
curl -s http://localhost:9091/metrics | head -n 50 || true

echo "Scale test baseline run complete."
