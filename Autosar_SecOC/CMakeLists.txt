cmake_minimum_required(VERSION 3.20)
set(CMAKE_BUILD_TYPE Debug)


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
elseif(UNIX)
    file(GLOB_RECURSE SCR_FILES CONFIGURE_DEPENDS  "source/*.c")
    file(GLOB_RECURSE HDR_FILES CONFIGURE_DEPENDS  "include/*.h")
    # Remove Windows-specific files on Linux
    list(REMOVE_ITEM SCR_FILES "${CMAKE_SOURCE_DIR}/source/Ethernet/ethernet_windows.c")
    list(REMOVE_ITEM HDR_FILES "${CMAKE_SOURCE_DIR}/include/Ethernet/ethernet_windows.h")
    set( CMAKE_CXX_COMPILER "/usr/bin/g++" )
    set( CMAKE_C_COMPILER "/usr/bin/gcc" )
    set (CMAKE_GENERATOR "Unix Makefiles" CACHE INTERNAL "" FORCE)
endif()

project(SecOC)

find_package(Threads REQUIRED)

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

# Link liboqs library
target_link_libraries(SecOC ${LIBOQS_LIBRARY})

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
    # Link Winsock2 library for Windows sockets
    target_link_libraries(SecOC ws2_32)
elseif(UNIX)
    target_compile_definitions(SecOC PUBLIC LINUX)
endif()


# Adding unit tests
include(CTest)

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
)
FetchContent_MakeAvailable(googletest)

# Extract include directories from main target SecOC
set(headerDirs "")
get_target_property(headerDirs SecOC INCLUDE_DIRECTORIES)

# Print include directories one per line for cppchecke extension 
string(REPLACE ";"  "\n" cppincludes "${headerDirs}")
file(WRITE "${CMAKE_SOURCE_DIR}/build/cppcheck_includes.txt" ${cppincludes})

# Create static library for linking with gtest
add_library(SecOCLib STATIC ${SCR_FILES} ${HDR_FILES})
target_include_directories(SecOCLib PUBLIC ${headerDirs} ${LIBOQS_INCLUDE_DIR})
target_link_libraries(SecOCLib ${LIBOQS_LIBRARY})

# Create shared library for linking with the GUI
add_library(SecOCLibShared SHARED ${SCR_FILES} ${HDR_FILES})
target_include_directories(SecOCLibShared PUBLIC ${headerDirs} ${LIBOQS_INCLUDE_DIR})
target_link_libraries(SecOCLibShared ${LIBOQS_LIBRARY})

# Platform-specific configurations for libraries
if(WIN32)
    target_compile_definitions(SecOCLib PUBLIC WINDOWS)
    target_compile_definitions(SecOCLibShared PUBLIC WINDOWS)
    target_link_libraries(SecOCLib ws2_32)
    target_link_libraries(SecOCLibShared ws2_32)
elseif(UNIX)
    target_compile_definitions(SecOCLib PUBLIC LINUX)
    target_compile_definitions(SecOCLibShared PUBLIC LINUX)
endif()

# Add our test folder cmakelists
add_subdirectory(test)







