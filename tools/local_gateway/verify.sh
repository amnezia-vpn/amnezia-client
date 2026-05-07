#!/usr/bin/env bash
# Smoke-test routes used by AmneziaVPN against a running local_gateway.
# Prerequisite: in another terminal: cd tools/local_gateway && go run .
# Usage: ./verify.sh [base_url]   default: http://127.0.0.1:8080
set -euo pipefail
BASE="${1:-http://127.0.0.1:8080}"

echo "== local_gateway verify base: ${BASE} =="

echo "== GET / =="
curl -sfS "$BASE/" | head -n 2

echo "== GET updater follow-up =="
curl -sfS "$BASE/VERSION" | head -c 20
echo
curl -sfS -o /dev/null -w "CHANGELOG %{http_code}\n" "$BASE/CHANGELOG"
curl -sfS -o /dev/null -w "RELEASE_DATE %{http_code}\n" "$BASE/RELEASE_DATE"

echo "== POST /v1/* (empty JSON) =="
for path in account_info services config news renewal_link updater_endpoint revoke_config revoke_native_config; do
  code=$(curl -sS -o /dev/null -w "%{http_code}" -X POST "$BASE/v1/$path" \
    -H "Content-Type: application/json" -d '{}')
  echo "POST /v1/$path -> HTTP $code"
  if [[ "$code" != "200" ]]; then
    echo "expected 200"; exit 1
  fi
done

echo "== POST pairing bad payload -> 400 =="
code=$(curl -sS -o /dev/null -w "%{http_code}" -X POST "$BASE/api/v1/generate_qr" \
  -H "Content-Type: application/json" -d '{}')
echo "POST /api/v1/generate_qr (invalid) -> HTTP $code"
[[ "$code" == "400" ]]

echo "OK"
