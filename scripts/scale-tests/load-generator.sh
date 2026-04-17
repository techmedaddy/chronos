#!/usr/bin/env bash
set -euo pipefail

API_URL="${API_URL:-http://localhost:8080}"
TOKEN="${TOKEN:-dev-token}"
COUNT="${COUNT:-1000}"

for i in $(seq 1 "$COUNT"); do
  curl -s -X POST "$API_URL/jobs" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"name":"job-'"$i"'","task_type":"sample.echo","queue_name":"main_queue","payload":{"n":'"$i"'}}' >/dev/null || true

done

echo "Submitted $COUNT jobs"
