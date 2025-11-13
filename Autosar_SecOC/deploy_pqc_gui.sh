#!/bin/bash
# PQC-Enabled SecOC GUI Deployment Script
# Run this in MSYS2 MinGW 64-bit terminal

set -e  # Exit on error

echo "========================================"
echo "PQC-Enabled SecOC Deployment Script"
echo "========================================"
echo ""

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found!"
    echo "Please run this script from the Autosar_SecOC directory"
    exit 1
fi

# Check if liboqs is built
if [ ! -f "external/liboqs/build/lib/liboqs.a" ]; then
    echo "❌ liboqs not found!"
    echo "Please build liboqs first:"
    echo "  cd external/liboqs"
    echo "  mkdir -p build && cd build"
    echo "  cmake -G \"MinGW Makefiles\" -DCMAKE_BUILD_TYPE=Release -DOQS_USE_OPENSSL=OFF .."
    echo "  mingw32-make -j4"
    exit 1
fi

echo "✓ liboqs found"
echo ""

# Step 1: Clean previous build
echo "Step 1: Cleaning previous build..."
cd build
rm -rf CMakeFiles CMakeCache.txt cmake_install.cmake Makefile _deps Testing 2>/dev/null || true
echo "✓ Build directory cleaned"
echo ""

# Step 2: Configure with CMake
echo "Step 2: Configuring CMake..."
cmake -G "MinGW Makefiles" ..
if [ $? -ne 0 ]; then
    echo "❌ CMake configuration failed!"
    exit 1
fi
echo "✓ CMake configured successfully"
echo ""

# Step 3: Build the project
echo "Step 3: Building SecOC with PQC support..."
mingw32-make -j4
if [ $? -ne 0 ]; then
    echo "❌ Build failed!"
    exit 1
fi
echo "✓ Build completed successfully"
echo ""

# Step 4: Verify library exists
echo "Step 4: Verifying build artifacts..."
if [ ! -f "libSecOCLibShared.dll" ]; then
    echo "❌ libSecOCLibShared.dll not found!"
    exit 1
fi

DLL_SIZE=$(stat -f%z "libSecOCLibShared.dll" 2>/dev/null || stat -c%s "libSecOCLibShared.dll" 2>/dev/null)
echo "✓ libSecOCLibShared.dll found (${DLL_SIZE} bytes)"
echo ""

# Step 5: Run the GUI
echo "========================================"
echo "Build Complete! Launching GUI..."
echo "========================================"
echo ""
echo "Features available:"
echo "  ✓ MAC Mode (HMAC authentication)"
echo "  ✓ PQC Mode (ML-DSA-65 signatures)"
echo "  ✓ Attack simulation"
echo "  ✓ Ethernet Gateway focus"
echo ""
echo "To enable PQC mode:"
echo "  1. Check 'Enable PQC Mode (ML-DSA-65 Signatures)'"
echo "  2. Click 'Accelerate' to authenticate"
echo "  3. Observe ~3300 byte signatures vs ~8 byte MAC"
echo ""

cd ../GUI
python simple_gui.py

echo ""
echo "GUI closed."
