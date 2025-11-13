#!/bin/bash
# Build liboqs (Open Quantum Safe library)
# Run this in MSYS2 MinGW 64-bit terminal

set -e  # Exit on error

echo "========================================"
echo "Building liboqs (Open Quantum Safe)"
echo "========================================"
echo ""

# Check if we're in the right directory
if [ ! -d "external/liboqs" ]; then
    echo "Error: external/liboqs directory not found!"
    echo "Please run this script from the Autosar_SecOC directory"
    exit 1
fi

cd external/liboqs

# Clean previous build
if [ -d "build" ]; then
    echo "Cleaning previous build..."
    rm -rf build
fi

# Create build directory
mkdir -p build
cd build

echo "Configuring liboqs with CMake..."
echo "  - Generator: MinGW Makefiles"
echo "  - Build Type: Release"
echo "  - OpenSSL: OFF (standalone)"
echo ""

cmake -G "MinGW Makefiles" \
      -DCMAKE_BUILD_TYPE=Release \
      -DOQS_USE_OPENSSL=OFF \
      ..

if [ $? -ne 0 ]; then
    echo "❌ CMake configuration failed!"
    echo ""
    echo "Make sure you are running this in MSYS2 MinGW 64-bit terminal"
    echo "and that cmake and mingw32-make are available."
    exit 1
fi

echo ""
echo "Building liboqs (this may take a few minutes)..."
mingw32-make -j4

if [ $? -ne 0 ]; then
    echo "❌ Build failed!"
    exit 1
fi

echo ""
echo "========================================"
echo "✓ liboqs built successfully!"
echo "========================================"
echo ""
echo "Library location: external/liboqs/build/lib/liboqs.a"
echo ""
echo "Supported algorithms:"
echo "  ✓ ML-KEM-768 (Key Encapsulation)"
echo "  ✓ ML-DSA-65 (Digital Signatures)"
echo ""
echo "Next step: Run ./deploy_pqc_gui.sh to build SecOC and launch GUI"
