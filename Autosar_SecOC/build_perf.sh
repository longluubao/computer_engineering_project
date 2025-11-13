#!/bin/bash
# Build and run performance benchmark

echo "Building performance benchmark..."

gcc -o test_perf.exe test_performance.c \
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
    echo "Build successful! Running benchmark..."
    echo ""
    ./test_perf.exe
else
    echo "Build failed!"
    exit 1
fi
