#!/bin/bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
APP_BIN="${PROJECT_DIR}/deploy/build/client/AmneziaVPN.app/Contents/MacOS/AmneziaVPN"

if [[ ! -x "$APP_BIN" ]]; then
  echo "Собранный macOS-клиент не найден: $APP_BIN" >&2
  echo "Сначала один раз собери проект, затем используй этот скрипт для повторных запусков без rebuild." >&2
  exit 1
fi

cd "$PROJECT_DIR"

# Проект собирает macOS-клиент как x86_64 bundle, поэтому на Apple Silicon
# бинарник нужно запускать через Rosetta.
if [[ "$(uname -m)" == "arm64" ]]; then
  exec arch -x86_64 "$APP_BIN" "$@"
fi

exec "$APP_BIN" "$@"

#      /Users/klej/Desktop/Random Software/AmneziaVPN/amnezia-client/run-macos-debug.command
# ./amnezia-client/run-macos-debug.command
