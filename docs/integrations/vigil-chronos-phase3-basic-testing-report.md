# Phase 3 Basic Testing Report (idempotency, dedupe, retention)

Date: 2026-04-18
Scope:
- concurrency race test (parallel duplicate submissions)
- replay determinism tests
- mismatch hard-fail behavior tests
- storage cleanup/retention behavior tests

---

## 1) Concurrency race test

Result: **PASS**

Method:
- 20 concurrent submissions with same `(tenant, endpoint, idempotency_key)` and same payload
- executed with Python concurrent HTTP client (single canonical body encoding)

Observed:
- response codes: `[202]` only
- unique remediation job ids: `1`

Interpretation:
- duplicate retries collapse to a single logical remediation job under parallel request conditions.

Note on initial shell harness:
- early shell-based xargs harness produced mixed 202/409 due to payload encoding drift in parallel shell expansion, not server semantic mismatch.
- canonical Python concurrency run confirms deterministic server behavior.

---

## 2) Replay determinism tests

Result: **PASS**

Observed:
- same key + same payload returns deterministic response body
- replay response includes `Idempotency-Replayed: true`

---

## 3) Mismatch hard-fail behavior tests

Result: **PASS**

Observed:
- same key + different payload returns `409`
- error code: `CHRONOS_IDEMPOTENCY_MISMATCH`

---

## 4) Storage cleanup / retention behavior tests

Result: **PASS**

Validated:
- repository-level TTL purge behavior for in-memory stores:
  - idempotency entries removable after TTL expiration
  - dedupe artifacts removable after TTL expiration
- retention endpoint:
  - `POST /v1/integrations/vigil/maintenance/retention:run` returns 200
  - includes `idempotencyRemoved` and `dedupeRemoved` counters

Default retention policy currently wired:
- idempotency TTL: 7 days
- dedupe TTL: 2 days

---

## Summary

- Concurrency race test: ✅
- Replay determinism: ✅
- Mismatch hard-fail: ✅
- Retention cleanup: ✅

Overall: **PASS**

## Exit criteria evaluation

**Exit criteria:** no duplicate remediation jobs from retries/network uncertainty.

Verdict: **PASS**

Under concurrent duplicate submissions and replay conditions, integration idempotency ensures one logical remediation job outcome with deterministic replay behavior.
