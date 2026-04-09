#!/bin/bash
# ═══════════════════════════════════════════════════════════════════════════
#  run_tests.sh — Full Test + Coverage + Cantata-Style Report Pipeline
# ═══════════════════════════════════════════════════════════════════════════
#
#  Usage:
#    bash run_tests.sh              # Full pipeline (build + test + coverage + report)
#    bash run_tests.sh --no-coverage # Skip coverage (faster build)
#    bash run_tests.sh --tests-only  # Skip build, just run tests and report
#
#  Output:
#    build/report/SecOC_Test_Report.html   — Cantata-style HTML report
#    build/report/coverage_html/           — Annotated source coverage (gcovr)
#    build/test_results/*.xml              — GTest XML results
#    build/coverage.json                   — Machine-readable coverage data
# ═══════════════════════════════════════════════════════════════════════════

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

BUILD_DIR="build"
RESULTS_DIR="${BUILD_DIR}/test_results"
REPORT_DIR="${BUILD_DIR}/report"
COVERAGE_JSON="${BUILD_DIR}/coverage.json"
REPORT_HTML="${REPORT_DIR}/SecOC_Test_Report.html"

ENABLE_COVERAGE=ON
TESTS_ONLY=OFF

# ── Parse arguments ──
for arg in "$@"; do
    case "$arg" in
        --no-coverage) ENABLE_COVERAGE=OFF ;;
        --tests-only)  TESTS_ONLY=ON ;;
        --help|-h)
            echo "Usage: bash run_tests.sh [--no-coverage] [--tests-only]"
            exit 0 ;;
    esac
done

echo "╔══════════════════════════════════════════════════════════════╗"
echo "║     AUTOSAR SecOC — Test & Coverage Pipeline                ║"
echo "║     Cantata-Style Report Generator                          ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""

# ── Step 1: Build with coverage ──
if [ "$TESTS_ONLY" = "OFF" ]; then
    echo "━━━ Step 1: Building with coverage=${ENABLE_COVERAGE} ━━━"
    rm -rf "${BUILD_DIR}/CMakeFiles" "${BUILD_DIR}/CMakeCache.txt"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    cmake -G "Unix Makefiles" \
          -DCMAKE_BUILD_TYPE=Debug \
          -DENABLE_COVERAGE=${ENABLE_COVERAGE} \
          -DMCAL_TARGET=PI4 \
          .. 2>&1 | tail -5
    make -j"$(nproc)" 2>&1 | tail -10
    cd "$SCRIPT_DIR"
    echo ""
fi

# ── Step 2: Clean old results ──
echo "━━━ Step 2: Preparing test results directory ━━━"
rm -rf "$RESULTS_DIR"
mkdir -p "$RESULTS_DIR"
mkdir -p "$REPORT_DIR"

# Reset coverage counters
if [ "$ENABLE_COVERAGE" = "ON" ]; then
    find "$BUILD_DIR" -name "*.gcda" -delete 2>/dev/null || true
fi
echo ""

# ── Step 3: Run tests ──
echo "━━━ Step 3: Running unit tests ━━━"
cd "$BUILD_DIR"

# Run tests with timeout, capture XML output.
# We use ctest but tests are configured to output XML via --gtest_output.
TEST_PASS=0
TEST_FAIL=0
TEST_TOTAL=0
FAILED_TESTS=""

# Get list of test executables
TEST_EXES=$(ctest --show-only=json-v1 2>/dev/null | python3 -c "
import json, sys
data = json.load(sys.stdin)
for t in data.get('tests', []):
    cmd = t.get('command', [])
    if cmd:
        print(cmd[0])
" 2>/dev/null || true)

if [ -z "$TEST_EXES" ]; then
    # Fallback: find test executables directly
    TEST_EXES=$(find test/ -maxdepth 1 -type f -executable 2>/dev/null || true)
    # Also check top-level
    for exe in Phase3_Complete_Test; do
        [ -f "$exe" ] && TEST_EXES="$TEST_EXES ./$exe"
    done
fi

for exe in $TEST_EXES; do
    if [ ! -f "$exe" ]; then continue; fi
    test_name=$(basename "$exe")
    echo -n "  Running ${test_name}... "

    # Skip DirectRxTests (requires separate receiver, blocks forever)
    if [ "$test_name" = "DirectRxTests" ] || [ "$test_name" = "DirectTxTests" ]; then
        echo "SKIPPED (requires separate receiver/transmitter)"
        continue
    fi

    # Run with timeout
    if timeout 30 "$exe" --gtest_output=xml:"${SCRIPT_DIR}/${RESULTS_DIR}/${test_name}.xml" > /dev/null 2>&1; then
        echo "PASS"
        TEST_PASS=$((TEST_PASS + 1))
    else
        echo "FAIL"
        TEST_FAIL=$((TEST_FAIL + 1))
        FAILED_TESTS="${FAILED_TESTS} ${test_name}"
    fi
    TEST_TOTAL=$((TEST_TOTAL + 1))
done

cd "$SCRIPT_DIR"
echo ""
echo "  Test Results: ${TEST_PASS}/${TEST_TOTAL} suites passed"
if [ -n "$FAILED_TESTS" ]; then
    echo "  Failed:${FAILED_TESTS}"
fi
echo ""

# ── Step 4: Generate coverage data ──
if [ "$ENABLE_COVERAGE" = "ON" ]; then
    echo "━━━ Step 4: Generating coverage data ━━━"

    # gcovr JSON for the report generator
    gcovr --root . \
          --filter "source/" \
          --exclude "source/Mcal/" \
          --exclude "source/Scheduler/" \
          --exclude "source/GUIInterface/" \
          --json "$COVERAGE_JSON" \
          2>/dev/null || echo "WARNING: gcovr JSON generation failed"

    # gcovr HTML for annotated source browsing
    mkdir -p "${REPORT_DIR}/coverage_html"
    gcovr --root . \
          --filter "source/" \
          --exclude "source/Mcal/" \
          --exclude "source/Scheduler/" \
          --exclude "source/GUIInterface/" \
          --html-details "${REPORT_DIR}/coverage_html/index.html" \
          --html-title "SecOC Coverage - Annotated Source" \
          2>/dev/null || echo "WARNING: gcovr HTML generation failed"

    echo "  Coverage JSON: ${COVERAGE_JSON}"
    echo "  Coverage HTML: ${REPORT_DIR}/coverage_html/index.html"
    echo ""
fi

# ── Step 5: Generate Cantata-style report ──
echo "━━━ Step 5: Generating Cantata-style HTML report ━━━"
python3 test/generate_report.py \
    --test-results-dir "$RESULTS_DIR" \
    --coverage-json "$COVERAGE_JSON" \
    --output "$REPORT_HTML"

echo ""
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║  Pipeline complete!                                         ║"
echo "║                                                             ║"
echo "║  Report: ${REPORT_HTML}          ║"
echo "║  Open in browser to view Cantata-style results.             ║"
echo "╚══════════════════════════════════════════════════════════════╝"
