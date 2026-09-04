#!/usr/bin/env bash
# Build + run the client in one shot.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

./scripts/build.sh
./scripts/run-client.sh
