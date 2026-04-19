cmake_minimum_required(VERSION 3.20)
set(CMAKE_BUILD_TYPE Debug)

# ---- Code Coverage Option ----
option(ENABLE_COVERAGE "Build with code coverage instrumentation (gcov)" OFF)
option(BUILD_TESTS "Build GoogleTest unit tests (pulls googletest via FetchContent)" ON)
if(ENABLE_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    message(STATUS "Code coverage: ENABLED (gcov)")
endif()

# ---- MCAL Target Selection ----
# Usage: cmake -DMCAL_TARGET=PI4 ..   (or SIM for simulation)
# Default: PI4 on Linux, SIM on Windows
if(NOT DEFINED MCAL_TARGET)
    if(UNIX)
        set(MCAL_TARGET "PI4" CACHE STRING "MCAL target platform (PI4, SIM)")
    else()
        set(MCAL_TARGET "SIM" CACHE STRING "MCAL target platform (PI4, SIM)")
    endif()
endif()
message(STATUS "MCAL Target: ${MCAL_TARGET}")

if (WIN32)
    file(GLOB_RECURSE SCR_FILES CONFIGURE_DEPENDS "source/*.c")
    file(GLOB_RECURSE HDR_FILES CONFIGURE_DEPENDS "include/*.h")
    # Remove Linux-specific files
    list(REMOVE_ITEM SCR_FILES "${CMAKE_SOURCE_DIR}/source/Ethernet/ethernet.c")
    list(REMOVE_ITEM HDR_FILES "${CMAKE_SOURCE_DIR}/include/Ethernet/ethernet.h")
    list(REMOVE_ITEM SCR_FILES "${CMAKE_SOURCE_DIR}/source/Scheduler/scheduler.c")
    list(REMOVE_ITEM HDR_FILES "${CMAKE_SOURCE_DIR}/include/Scheduler/scheduler.h")
    # Remove Windows-specific files from the glob (they're not needed on Linux)
    list(REMOVE_ITEM SCR_FILES "${CMAKE_SOURCE_DIR}/source/Ethernet/ethernet_windows.c")
    list(REMOVE_ITEM HDR_FILES "${CMAKE_SOURCE_DIR}/include/Ethernet/ethernet_windows.h")
    # Add Windows-specific files back
    list(APPEND SCR_FILES "${CMAKE_SOURCE_DIR}/source/Ethernet/ethernet_windows.c")
    list(APPEND HDR_FILES "${CMAKE_SOURCE_DIR}/include/Ethernet/ethernet_windows.h")
    # Remove Pi4 MCAL files on Windows (use simulation CAN)
    file(GLOB_RECURSE MCAL_PI4_FILES "${CMAKE_SOURCE_DIR}/source/Mcal/Pi4/*.c")
    foreach(_f ${MCAL_PI4_FILES})
        list(REMOVE_ITEM SCR_FILES "${_f}")
    endforeach()
elseif(UNIX)
    file(GLOB_RECURSE SCR_FILES CONFIGURE_DEPENDS  "source/*.c")
    file(GLOB_RECURSE HDR_FILES CONFIGURE_DEPENDS  "include/*.h")
    # Remove Windows-specific files on Linux
    list(REMOVE_ITEM SCR_FILES "${CMAKE_SOURCE_DIR}/source/Ethernet/ethernet_windows.c")
    list(REMOVE_ITEM HDR_FILES "${CMAKE_SOURCE_DIR}/include/Ethernet/ethernet_windows.h")
    # Keep scheduler path disabled when OS task-based dispatch is active
    list(REMOVE_ITEM SCR_FILES "${CMAKE_SOURCE_DIR}/source/Scheduler/scheduler.c")
    list(REMOVE_ITEM HDR_FILES "${CMAKE_SOURCE_DIR}/include/Scheduler/scheduler.h")
    set( CMAKE_CXX_COMPILER "/usr/bin/g++" )
    set( CMAKE_C_COMPILER "/usr/bin/gcc" )
    set (CMAKE_GENERATOR "Unix Makefiles" CACHE INTERNAL "" FORCE)

    # ---- MCAL Target-specific source selection ----
    if(MCAL_TARGET STREQUAL "PI4")
        # Replace simulation CAN driver with SocketCAN Pi4 driver
        list(REMOVE_ITEM SCR_FILES "${CMAKE_SOURCE_DIR}/source/Can/Can.c")
        message(STATUS "MCAL: Using Pi4 SocketCAN driver (replacing simulation Can.c)")
    else()
        # SIM mode: remove Pi4-specific MCAL source files
        file(GLOB_RECURSE MCAL_PI4_FILES "${CMAKE_SOURCE_DIR}/source/Mcal/Pi4/*.c")
        foreach(_f ${MCAL_PI4_FILES})
            list(REMOVE_ITEM SCR_FILES "${_f}")
        endforeach()
        message(STATUS "MCAL: Using simulation mode (Can.c software queues)")
    endif()
endif()

project(SecOC)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# Add liboqs library path
set(LIBOQS_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/liboqs/build/include")
set(LIBOQS_LIB_DIR "${CMAKE_SOURCE_DIR}/external/liboqs/build/lib")
set(LIBOQS_LIBRARY "${LIBOQS_LIB_DIR}/liboqs.a")

# Check if liboqs exists
if(NOT EXISTS ${LIBOQS_LIBRARY})
    message(FATAL_ERROR "liboqs not found! Please build liboqs first in external/liboqs/build")
endif()


add_executable(SecOC ${SCR_FILES})

# Add liboqs include directory
target_include_directories(SecOC PUBLIC ${LIBOQS_INCLUDE_DIR})

# Link liboqs library (requires OpenSSL)
target_link_libraries(SecOC ${LIBOQS_LIBRARY} OpenSSL::Crypto OpenSSL::SSL)

if(THREADS_HAVE_PTHREAD_ARG)
  target_compile_options(SecOC PUBLIC "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(SecOC "${CMAKE_THREAD_LIBS_INIT}")
endif()

foreach (_file ${HDR_FILES})
    cmake_path(REMOVE_FILENAME _file)

    target_include_directories(SecOC PUBLIC ${_file})
endforeach()


if(WIN32)
    target_compile_definitions(SecOC PUBLIC WINDOWS)
    target_compile_definitions(SecOC PUBLIC MCAL_TARGET=MCAL_TARGET_SIM)
    # Link Winsock2 library for Windows sockets
    target_link_libraries(SecOC ws2_32)
elseif(UNIX)
    target_compile_definitions(SecOC PUBLIC LINUX)
    if(MCAL_TARGET STREQUAL "PI4")
        target_compile_definitions(SecOC PUBLIC MCAL_TARGET=MCAL_TARGET_PI4)
    else()
        target_compile_definitions(SecOC PUBLIC MCAL_TARGET=MCAL_TARGET_SIM)
    endif()
endif()


# Adding unit tests (skippable via -DBUILD_TESTS=OFF for static-analysis builds
# like CodeQL where FetchContent googletest's Makefile sub-build clashes with
# a Ninja parent build).
if(BUILD_TESTS)
    include(CTest)

    include(FetchContent)

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
    )
    FetchContent_MakeAvailable(googletest)
endif()

# Extract include directories from main target SecOC
set(headerDirs "")
get_target_property(headerDirs SecOC INCLUDE_DIRECTORIES)

# Print include directories one per line for cppchecke extension 
string(REPLACE ";"  "\n" cppincludes "${headerDirs}")
file(WRITE "${CMAKE_SOURCE_DIR}/build/cppcheck_includes.txt" ${cppincludes})

# Create static library for linking with gtest
add_library(SecOCLib STATIC ${SCR_FILES} ${HDR_FILES})
target_include_directories(SecOCLib PUBLIC ${headerDirs} ${LIBOQS_INCLUDE_DIR})
target_link_libraries(SecOCLib ${LIBOQS_LIBRARY} OpenSSL::Crypto OpenSSL::SSL)

# Create shared library for linking with the GUI
add_library(SecOCLibShared SHARED ${SCR_FILES} ${HDR_FILES})
target_include_directories(SecOCLibShared PUBLIC ${headerDirs} ${LIBOQS_INCLUDE_DIR})
target_link_libraries(SecOCLibShared ${LIBOQS_LIBRARY} OpenSSL::Crypto OpenSSL::SSL)

# Platform-specific configurations for libraries
if(WIN32)
    target_compile_definitions(SecOCLib PUBLIC WINDOWS)
    target_compile_definitions(SecOCLibShared PUBLIC WINDOWS)
    target_compile_definitions(SecOCLib PUBLIC MCAL_TARGET=MCAL_TARGET_SIM)
    target_compile_definitions(SecOCLibShared PUBLIC MCAL_TARGET=MCAL_TARGET_SIM)
    target_link_libraries(SecOCLib ws2_32)
    target_link_libraries(SecOCLibShared ws2_32)
elseif(UNIX)
    target_compile_definitions(SecOCLib PUBLIC LINUX)
    target_compile_definitions(SecOCLibShared PUBLIC LINUX)
    if(MCAL_TARGET STREQUAL "PI4")
        target_compile_definitions(SecOCLib PUBLIC MCAL_TARGET=MCAL_TARGET_PI4)
        target_compile_definitions(SecOCLibShared PUBLIC MCAL_TARGET=MCAL_TARGET_PI4)
    else()
        target_compile_definitions(SecOCLib PUBLIC MCAL_TARGET=MCAL_TARGET_SIM)
        target_compile_definitions(SecOCLibShared PUBLIC MCAL_TARGET=MCAL_TARGET_SIM)
    endif()
endif()

# Add Phase 3: Complete Ethernet Gateway Integration Test
add_executable(Phase3_Complete_Test test/test_phase3_complete_ethernet_gateway.c)
target_include_directories(Phase3_Complete_Test PUBLIC ${headerDirs} ${LIBOQS_INCLUDE_DIR})
target_link_libraries(Phase3_Complete_Test SecOCLib ${LIBOQS_LIBRARY} OpenSSL::Crypto OpenSSL::SSL)
if(WIN32)
    target_compile_definitions(Phase3_Complete_Test PUBLIC WINDOWS)
    target_link_libraries(Phase3_Complete_Test ws2_32)
elseif(UNIX)
    target_compile_definitions(Phase3_Complete_Test PUBLIC LINUX)
endif()
add_test(NAME Phase3_Complete_Integration COMMAND Phase3_Complete_Test)

# Add our test folder cmakelists (requires googletest, gated by BUILD_TESTS)
if(BUILD_TESTS)
    add_subdirectory(test)
endif()







