#!/bin/bash
# Unified Build and Run Script for AUTOSAR SecOC with PQC
# Combines: rebuild_pqc.sh + build_autosar_integration.sh + build_advanced_test.sh
# Platform: x86_64 (development) and Raspberry Pi 4 (deployment)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Detect platform
if [ "$(uname -m)" = "aarch64" ] || [ "$(uname -m)" = "armv7l" ]; then
    PLATFORM="RASPBERRY_PI"
    PLATFORM_FLAGS="-DRASPBERRY_PI -mcpu=cortex-a72"
else
    PLATFORM="X86_64"
    PLATFORM_FLAGS="-DWINDOWS"
fi

# Function: Print banner
print_banner() {
    echo -e "${CYAN}"
    echo "╔════════════════════════════════════════════════════════════╗"
    echo "║     AUTOSAR SecOC with Post-Quantum Cryptography           ║"
    echo "║            Unified Build & Run Script                      ║"
    echo "╚════════════════════════════════════════════════════════════╝"
    echo -e "${NC}"
    echo -e "${BLUE}Platform: $PLATFORM ($(uname -m))${NC}"
    echo ""
}

# Function: Check dependencies
check_dependencies() {
    echo -e "${YELLOW}📌 Checking dependencies...${NC}"

    if ! command -v gcc &> /dev/null; then
        echo -e "${RED}❌ GCC not found!${NC}"
        echo "   Install with: sudo apt-get install build-essential"
        return 1
    fi
    echo -e "${GREEN}✅ GCC found: $(gcc --version | head -n1)${NC}"

    if ! command -v cmake &> /dev/null; then
        echo -e "${RED}❌ CMake not found!${NC}"
        echo "   Install with: sudo apt-get install cmake"
        return 1
    fi
    echo -e "${GREEN}✅ CMake found${NC}"

    if ! command -v python3 &> /dev/null; then
        echo -e "${YELLOW}⚠️  Python3 not found (optional for visualization)${NC}"
    else
        echo -e "${GREEN}✅ Python3 found${NC}"
    fi

    return 0
}

# Function: Build liboqs
build_liboqs() {
    echo ""
    echo -e "${CYAN}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║              BUILDING LIBOQS LIBRARY                       ║${NC}"
    echo -e "${CYAN}╚════════════════════════════════════════════════════════════╝${NC}"
    echo ""

    if [ -f "external/liboqs/build/lib/liboqs.a" ]; then
        echo -e "${GREEN}✅ liboqs already built - skipping rebuild${NC}"
        echo -e "${YELLOW}   (To force rebuild: rm -rf external/liboqs/build)${NC}"
        return 0
    fi

    cd external/liboqs || { echo -e "${RED}❌ liboqs directory not found${NC}"; return 1; }

    # Clean previous build
    rm -rf build
    mkdir -p build
    cd build

    # Configure based on platform
    if [ "$PLATFORM" = "RASPBERRY_PI" ]; then
        echo -e "${BLUE}🎯 Configuring for Raspberry Pi 4...${NC}"
        cmake -GNinja \
            -DCMAKE_BUILD_TYPE=Release \
            -DBUILD_SHARED_LIBS=OFF \
            -DOQS_USE_OPENSSL=ON \
            -DOQS_DIST_BUILD=OFF \
            -DOQS_ENABLE_KEM_ml_kem_768=ON \
            -DOQS_ENABLE_SIG_ml_dsa_65=ON \
            -DCMAKE_C_FLAGS="-mcpu=cortex-a72 -mtune=cortex-a72 -O3 -fPIC" \
            ..
    else
        echo -e "${BLUE}🎯 Configuring for x86_64...${NC}"
        cmake -GNinja \
            -DCMAKE_BUILD_TYPE=Release \
            -DBUILD_SHARED_LIBS=OFF \
            -DOQS_USE_OPENSSL=ON \
            -DOQS_DIST_BUILD=ON \
            -DOQS_ENABLE_KEM_ml_kem_768=ON \
            -DOQS_ENABLE_SIG_ml_dsa_65=ON \
            ..
    fi

    # Build
    echo -e "${BLUE}🔨 Building liboqs...${NC}"
    ninja

    if [ $? -eq 0 ]; then
        cd ../../..
        echo ""
        echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
        echo -e "${GREEN}║              ✅ LIBOQS BUILD SUCCESSFUL                     ║${NC}"
        echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"
        return 0
    else
        cd ../../..
        echo -e "${RED}❌ liboqs build failed${NC}"
        return 1
    fi
}

# Function: Build AUTOSAR integration test
build_integration_test() {
    echo ""
    echo -e "${CYAN}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║         BUILDING AUTOSAR INTEGRATION TEST                  ║${NC}"
    echo -e "${CYAN}╚════════════════════════════════════════════════════════════╝${NC}"
    echo ""

    # Check liboqs
    if [ ! -f "external/liboqs/build/lib/liboqs.a" ]; then
        echo -e "${RED}❌ liboqs not found!${NC}"
        echo "   Building liboqs first..."
        build_liboqs || return 1
    fi

    echo -e "${BLUE}🔨 Compiling test_autosar_integration_comprehensive.c...${NC}"

    gcc -o test_autosar_integration.exe \
        test_autosar_integration_comprehensive.c \
        source/PQC/PQC.c \
        source/PQC/PQC_KeyExchange.c \
        source/Csm/Csm.c \
        source/Encrypt/encrypt.c \
        source/SecOC/SecOC.c \
        source/SecOC/FVM.c \
        source/SecOC/SecOC_Lcfg.c \
        source/SecOC/SecOC_PBcfg.c \
        source/PduR/PduR_Com.c \
        source/PduR/Pdur_SecOC.c \
        source/Com/Com.c \
        -I include \
        -I include/PQC \
        -I include/Csm \
        -I include/Encrypt \
        -I include/SecOC \
        -I include/PduR \
        -I include/Com \
        -I external/liboqs/build/include \
        -L external/liboqs/build/lib \
        -loqs \
        -lm \
        -lpthread \
        $PLATFORM_FLAGS

    if [ $? -eq 0 ]; then
        echo ""
        echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
        echo -e "${GREEN}║           ✅ INTEGRATION TEST BUILD SUCCESSFUL              ║${NC}"
        echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"
        echo ""
        echo -e "${BLUE}📁 Output: test_autosar_integration.exe${NC}"
        return 0
    else
        echo ""
        echo -e "${RED}╔════════════════════════════════════════════════════════════╗${NC}"
        echo -e "${RED}║              ❌ BUILD FAILED!                               ║${NC}"
        echo -e "${RED}╚════════════════════════════════════════════════════════════╝${NC}"
        return 1
    fi
}

# Function: Build advanced test
build_advanced_test() {
    echo ""
    echo -e "${CYAN}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║           BUILDING ADVANCED PQC METRICS TEST               ║${NC}"
    echo -e "${CYAN}╚════════════════════════════════════════════════════════════╝${NC}"
    echo ""

    # Check liboqs
    if [ ! -f "external/liboqs/build/lib/liboqs.a" ]; then
        echo -e "${RED}❌ liboqs not found!${NC}"
        echo "   Building liboqs first..."
        build_liboqs || return 1
    fi

    echo -e "${BLUE}🔨 Compiling test_pqc_advanced.c...${NC}"

    gcc -o test_pqc_advanced.exe \
        test_pqc_advanced.c \
        source/PQC/PQC.c \
        source/PQC/PQC_KeyExchange.c \
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
        -lm \
        $PLATFORM_FLAGS

    if [ $? -eq 0 ]; then
        echo ""
        echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
        echo -e "${GREEN}║         ✅ ADVANCED TEST BUILD SUCCESSFUL                   ║${NC}"
        echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"
        echo ""
        echo -e "${BLUE}📁 Output: test_pqc_advanced.exe${NC}"
        return 0
    else
        echo ""
        echo -e "${RED}❌ Advanced test build failed${NC}"
        return 1
    fi
}

# Function: Run tests
run_tests() {
    echo ""
    echo -e "${CYAN}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║                  RUNNING TESTS                             ║${NC}"
    echo -e "${CYAN}╚════════════════════════════════════════════════════════════╝${NC}"
    echo ""

    if [ ! -f "test_autosar_integration.exe" ]; then
        echo -e "${RED}❌ test_autosar_integration.exe not found!${NC}"
        echo "   Build it first (option 2)"
        return 1
    fi

    echo -e "${BLUE}🚀 Running AUTOSAR integration test...${NC}"
    echo ""

    ./test_autosar_integration.exe

    if [ $? -eq 0 ]; then
        echo ""
        echo -e "${GREEN}✅ Tests completed successfully!${NC}"

        if [ -f "autosar_integration_results.csv" ]; then
            echo -e "${GREEN}✅ Results exported to: autosar_integration_results.csv${NC}"
        fi

        return 0
    else
        echo -e "${RED}❌ Tests failed${NC}"
        return 1
    fi
}

# Function: Run visualization
run_visualization() {
    echo ""
    echo -e "${CYAN}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║              LAUNCHING VISUALIZATION                       ║${NC}"
    echo -e "${CYAN}╚════════════════════════════════════════════════════════════╝${NC}"
    echo ""

    if [ ! -f "pqc_premium_dashboard.py" ]; then
        echo -e "${RED}❌ pqc_premium_dashboard.py not found!${NC}"
        return 1
    fi

    if ! command -v python3 &> /dev/null; then
        echo -e "${RED}❌ Python3 not found!${NC}"
        echo "   Install with: sudo apt-get install python3"
        return 1
    fi

    echo -e "${BLUE}🎨 Launching premium dashboard...${NC}"
    echo ""

    python3 pqc_premium_dashboard.py
}

# Function: Show menu
show_menu() {
    echo ""
    echo -e "${CYAN}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║                      MAIN MENU                             ║${NC}"
    echo -e "${CYAN}╚════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "${YELLOW}1)${NC} Build liboqs library"
    echo -e "${YELLOW}2)${NC} Build AUTOSAR integration test ${GREEN}(RECOMMENDED)${NC}"
    echo -e "${YELLOW}3)${NC} Build advanced PQC metrics test"
    echo -e "${YELLOW}4)${NC} Build ALL ${GREEN}(liboqs + integration + advanced)${NC}"
    echo -e "${YELLOW}5)${NC} Run tests"
    echo -e "${YELLOW}6)${NC} Launch visualization dashboard"
    echo -e "${YELLOW}7)${NC} Quick demo ${CYAN}(build all + run + visualize)${NC}"
    echo -e "${YELLOW}0)${NC} Exit"
    echo ""
}

# Function: Quick demo
quick_demo() {
    echo -e "${CYAN}════════════════════════════════════════════════════════════${NC}"
    echo -e "${CYAN}              QUICK DEMO - BUILD ALL & RUN                  ${NC}"
    echo -e "${CYAN}════════════════════════════════════════════════════════════${NC}"

    build_liboqs || return 1
    build_integration_test || return 1
    run_tests || return 1

    echo ""
    echo -e "${YELLOW}Launching visualization dashboard...${NC}"
    run_visualization
}

# Function: Show usage
show_usage() {
    echo ""
    echo -e "${CYAN}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║                    USAGE INSTRUCTIONS                      ║${NC}"
    echo -e "${CYAN}╚════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "${YELLOW}Available commands:${NC}"
    echo ""
    echo -e "  ${GREEN}bash build_and_run.sh all${NC}          - Build everything (liboqs + tests)"
    echo -e "  ${GREEN}bash build_and_run.sh liboqs${NC}       - Build only liboqs library"
    echo -e "  ${GREEN}bash build_and_run.sh integration${NC}  - Build AUTOSAR integration test"
    echo -e "  ${GREEN}bash build_and_run.sh advanced${NC}     - Build advanced PQC metrics test"
    echo -e "  ${GREEN}bash build_and_run.sh test${NC}         - Run all tests"
    echo -e "  ${GREEN}bash build_and_run.sh viz${NC}          - Launch visualization dashboard"
    echo -e "  ${GREEN}bash build_and_run.sh demo${NC}         - Quick demo (build + test + viz)"
    echo ""
    echo -e "${YELLOW}Examples:${NC}"
    echo -e "  ${BLUE}# Build everything:${NC}"
    echo -e "  bash build_and_run.sh all"
    echo ""
    echo -e "  ${BLUE}# Run tests after building:${NC}"
    echo -e "  bash build_and_run.sh all && bash build_and_run.sh test"
    echo ""
    echo -e "  ${BLUE}# Full demo:${NC}"
    echo -e "  bash build_and_run.sh demo"
    echo ""
}

# Main execution
main() {
    print_banner
    check_dependencies || exit 1

    # If arguments provided, run non-interactive mode
    if [ $# -gt 0 ]; then
        case "$1" in
            "liboqs")
                build_liboqs
                ;;
            "integration")
                build_integration_test
                ;;
            "advanced")
                build_advanced_test
                ;;
            "all")
                build_liboqs && build_integration_test && build_advanced_test
                ;;
            "test")
                run_tests
                ;;
            "viz")
                run_visualization
                ;;
            "demo")
                quick_demo
                ;;
            *)
                echo -e "${RED}❌ Unknown command: $1${NC}"
                show_usage
                exit 1
                ;;
        esac
        exit $?
    fi

    # No arguments - show usage and exit (interactive mode disabled for MINGW64 compatibility)
    show_usage
    exit 0

    # Interactive mode (disabled - use command-line arguments instead)
    while true; do
        show_menu
        echo -n "Select option (0-7): "
        read choice

        case $choice in
            1)
                build_liboqs
                ;;
            2)
                build_integration_test
                ;;
            3)
                build_advanced_test
                ;;
            4)
                build_liboqs && build_integration_test && build_advanced_test
                ;;
            5)
                run_tests
                ;;
            6)
                run_visualization
                ;;
            7)
                quick_demo
                ;;
            0)
                echo ""
                echo -e "${GREEN}Thank you for using AUTOSAR SecOC PQC! 👋${NC}"
                echo ""
                exit 0
                ;;
            *)
                echo -e "${RED}Invalid option. Please try again.${NC}"
                ;;
        esac

        echo ""
        echo -n "Press Enter to continue..."
        read dummy
    done
}

# Run main
main "$@"
