#!/usr/bin/env bash
# Build the Doxygen HTML documentation.
#
# Usage: scripts/docs.sh [--open]
#   --open   open the generated docs in a browser after building
#
# Output: build/debug/docs/html/index.html
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"

cmake --preset debug
cmake --build build/debug --target docs

echo "Docs: build/debug/docs/html/index.html"

if [[ "${1:-}" == "--open" ]]; then
    xdg-open build/debug/docs/html/index.html
fi
