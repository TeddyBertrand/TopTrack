#!/usr/bin/env bash
# Configure + build. Usage: scripts/build.sh [--server-only]
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

CLIENT_FLAG="ON"
if [[ "${1:-}" == "--server-only" ]]; then
  CLIENT_FLAG="OFF"
fi

cmake -S . -B build -DTOPTRACK_BUILD_CLIENT="$CLIENT_FLAG"
cmake --build build -j"$(nproc)"
