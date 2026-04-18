#!/usr/bin/env bash
# Build the Integrated Simulation Environment on both x86_64 Linux/Windows
# (MinGW) and Raspberry Pi 4 (Linux/aarch64 or armv7l), matching the
# convention used by Autosar_SecOC/build_and_run.sh.
#
# Usage:
#     bash build.sh                 # normal build
#     bash build.sh -c              # clean rebuild
#     bash build.sh -DOPT=VAL       # pass extra -D to cmake

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

# ---------------------------------------------------------------------------
# Platform detection — same logic as build_and_run.sh
# ---------------------------------------------------------------------------
UNAME_M="$(uname -m)"
UNAME_S="$(uname -s)"
case "$UNAME_M" in
    aarch64|armv7l)
        PLATFORM="RASPBERRY_PI"
        PI_FLAGS=( "-DCMAKE_C_FLAGS=-mcpu=cortex-a72 -O3 -fPIC" )
        ;;
    *)
        PLATFORM="X86_64"
        PI_FLAGS=( )
        ;;
esac

case "$UNAME_S" in
    MINGW*|MSYS*|CYGWIN*)
        GENERATOR="MinGW Makefiles"
        ;;
    *)
        GENERATOR="Unix Makefiles"
        ;;
esac

# ---------------------------------------------------------------------------
# liboqs prerequisite. Reuse the main project's build if available, else
# configure & build it in place with the same flags used by build_and_run.sh.
# ---------------------------------------------------------------------------
LIBOQS_A="${AUTOSAR_ROOT}/external/liboqs/build/lib/liboqs.a"
if [[ ! -f "$LIBOQS_A" ]]; then
    echo "[ise] liboqs not built — configuring & building in-place"
    pushd "${AUTOSAR_ROOT}/external/liboqs" >/dev/null
    mkdir -p build && cd build
    if [[ "$PLATFORM" == "RASPBERRY_PI" ]]; then
        cmake -GNinja \
            -DCMAKE_BUILD_TYPE=Release \
            -DBUILD_SHARED_LIBS=OFF \
            -DOQS_USE_OPENSSL=ON \
            -DOQS_ENABLE_KEM_ml_kem_768=ON \
            -DOQS_ENABLE_SIG_ml_dsa_65=ON \
            -DOQS_BUILD_ONLY_LIB=ON \
            -DCMAKE_C_FLAGS="-mcpu=cortex-a72 -O3 -fPIC" ..
    else
        cmake -GNinja \
            -DCMAKE_BUILD_TYPE=Release \
            -DBUILD_SHARED_LIBS=OFF \
            -DOQS_USE_OPENSSL=ON \
            -DOQS_DIST_BUILD=ON \
            -DOQS_ENABLE_KEM_ml_kem_768=ON \
            -DOQS_ENABLE_SIG_ml_dsa_65=ON \
            -DOQS_BUILD_ONLY_LIB=ON ..
    fi
    ninja
    popd >/dev/null
fi

# ---------------------------------------------------------------------------
# Configure & build the ISE itself.
# ---------------------------------------------------------------------------
if [[ $CLEAN -eq 1 && -d "$BUILD_DIR" ]]; then
    echo "[ise] cleaning $BUILD_DIR"
    rm -rf "$BUILD_DIR"
fi
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "[ise] platform=$PLATFORM ($UNAME_M / $UNAME_S)"
echo "[ise] configuring with generator '$GENERATOR'"
cmake -G "$GENERATOR" "${PI_FLAGS[@]}" "${EXTRA_ARGS[@]}" ..

echo "[ise] building"
cmake --build . -j"$(nproc 2>/dev/null || echo 4)"

echo
echo "[ise] build OK — try:"
echo "      ${BUILD_DIR}/ise_runner --scenario baseline --iterations 50 --protection pqc"
