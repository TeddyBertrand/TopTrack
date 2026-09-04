#!/usr/bin/env bash
# Build + run the server in one shot. Usage: scripts/dev-server.sh [port]
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

./scripts/build.sh --server-only
./scripts/run-server.sh "${1:-7777}"
