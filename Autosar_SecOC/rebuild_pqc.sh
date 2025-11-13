#!/bin/bash
# Script to rebuild SecOC with PQC integration

echo "============================================"
echo "Rebuilding AUTOSAR SecOC with PQC Support"
echo "============================================"
echo ""

# Navigate to build directory
cd build

# Clean previous build completely
echo "Cleaning previous build..."
rm -rf CMakeFiles CMakeCache.txt cmake_install.cmake Makefile
rm -rf _deps Testing CTestTestfile.cmake
rm -rf *.a *.exe *.dll

# Reconfigure with CMake (explicitly specify MinGW Makefiles)
echo "Configuring CMake..."
cmake -G "MinGW Makefiles" ..

# Build the project
echo "Building SecOC with PQC..."
mingw32-make -j4

echo ""
echo "============================================"
echo "Build Complete!"
echo "============================================"
echo ""
echo "Executables:"
echo "  - SecOC.exe (main executable)"
echo "  - libSecOCLibShared.dll (for GUI)"
echo ""
echo "To run the GUI:"
echo "  cd GUI"
echo "  python simple_gui.py"
