#!/usr/bin/env bash
# Generate an HTML coverage report using clang's instrumentation-based coverage.
#
# Usage: scripts/coverage.sh [--open]
#   --open   open the report in a browser after generation
#
# Output: build/coverage/report/index.html
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
BUILD_DIR=$ROOT/build/coverage
REPORT_DIR=$BUILD_DIR/report
PROFRAW_DIR=$BUILD_DIR/tests/profraw
PROFDATA=$BUILD_DIR/default.profdata
TEST_BIN=$BUILD_DIR/tests/test_md2v

# Exclude third-party headers and test source from the report
IGNORE_REGEX='vcpkg_installed|/tests/'

cd "$ROOT"

cmake --preset coverage
cmake --build --preset coverage

# Each CTest invocation (one per discovered test case) writes its own .profraw.
# Use an absolute path so the binary writes files relative to PROFRAW_DIR
# regardless of CTest's working directory.
mkdir -p "$PROFRAW_DIR"
find "$PROFRAW_DIR" -name "*.profraw" -delete
LLVM_PROFILE_FILE="$PROFRAW_DIR/%p.profraw" ctest --preset coverage --quiet

# Merge all profraw files from this run
llvm-profdata merge -sparse "$PROFRAW_DIR"/*.profraw -o "$PROFDATA"

# Summary to stdout
echo ""
echo "=== Coverage summary ==="
llvm-cov report "$TEST_BIN" \
    --instr-profile="$PROFDATA" \
    --ignore-filename-regex="$IGNORE_REGEX"

# Full annotated HTML report
llvm-cov show "$TEST_BIN" \
    --instr-profile="$PROFDATA" \
    --format=html \
    --output-dir="$REPORT_DIR" \
    --ignore-filename-regex="$IGNORE_REGEX" \
    --show-line-counts-or-regions \
    --show-expansions

echo ""
echo "Report: $REPORT_DIR/index.html"

if [[ "${1:-}" == "--open" ]]; then
    xdg-open "$REPORT_DIR/index.html"
fi
