#!/usr/bin/env bash
# Usage: scripts/run-server.sh [port]
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
exec ./build/server/toptrack_server "${1:-7777}"
