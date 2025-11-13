#!/bin/bash
# Build and run PQC test program

echo "Building PQC test program..."

# Compile the test
gcc -o test_pqc.exe test_pqc.c \
    source/PQC/PQC.c \
    source/Csm/Csm.c \
    source/Encrypt/encrypt.c \
    -I include \
    -I include/PQC \
    -I include/Csm \
    -I include/Encrypt \
    -I include/SecOC \
    -I external/liboqs/build/include \
    -L external/liboqs/build/lib \
    -loqs \
    -DWINDOWS

if [ $? -eq 0 ]; then
    echo "Build successful! Running test..."
    echo ""
    ./test_pqc.exe
else
    echo "Build failed!"
    exit 1
fi
