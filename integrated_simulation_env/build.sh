#!/usr/bin/env bash
# Build the Integrated Simulation Environment.
#
# Usage:  bash build.sh [-c|--clean] [-D<cmake_define>]
#
# The script assumes the main Autosar_SecOC project has been configured
# so that liboqs and SecOCLib exist.

set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${HERE}/build"
AUTOSAR_ROOT="${HERE}/../Autosar_SecOC"

CLEAN=0
EXTRA_ARGS=()
for arg in "$@"; do
    case "$arg" in
        -c|--clean) CLEAN=1 ;;
        *)          EXTRA_ARGS+=("$arg") ;;
    esac
done

# Make sure liboqs is available. The main project script does this too,
# but we don't want to fail mysteriously inside cmake configure.
if [[ ! -f "${AUTOSAR_ROOT}/external/liboqs/build/lib/liboqs.a" ]]; then
    echo "[ise] liboqs not built — running ${AUTOSAR_ROOT}/build_liboqs.sh"
    (cd "${AUTOSAR_ROOT}" && bash build_liboqs.sh)
fi

if [[ $CLEAN -eq 1 && -d "$BUILD_DIR" ]]; then
    echo "[ise] cleaning $BUILD_DIR"
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

if [[ "$(uname -s)" == "Linux" || "$(uname -s)" == "Darwin" ]]; then
    GENERATOR="Unix Makefiles"
else
    GENERATOR="MinGW Makefiles"
fi

echo "[ise] configuring with generator '$GENERATOR'"
cmake -G "$GENERATOR" .. "${EXTRA_ARGS[@]}"
echo "[ise] building"
cmake --build . -j"$(nproc 2>/dev/null || echo 4)"

echo
echo "[ise] build OK — try:"
echo "      ${BUILD_DIR}/ise_runner --scenario baseline --iterations 50 --protection pqc"
