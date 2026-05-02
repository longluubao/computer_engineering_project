cmake_minimum_required(VERSION 3.20)
project(integrated_simulation_env C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

#
# The ISE re-uses the real SecOC project as a dependency. We expect the
# main CMakeLists at Autosar_SecOC/ to have been configured & built at
# least once so that SecOCLib (static) and liboqs exist.
#
set(AUTOSAR_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../Autosar_SecOC"
    CACHE PATH "Path to the Autosar_SecOC directory")

if(NOT EXISTS "${AUTOSAR_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR
        "Expected to find ${AUTOSAR_ROOT}/CMakeLists.txt — is AUTOSAR_ROOT correct?")
endif()

# liboqs discovery (same layout as the main project).
set(LIBOQS_INCLUDE_DIR "${AUTOSAR_ROOT}/external/liboqs/build/include")
set(LIBOQS_LIBRARY     "${AUTOSAR_ROOT}/external/liboqs/build/lib/liboqs.a")
if(NOT EXISTS ${LIBOQS_LIBRARY})
    message(FATAL_ERROR
        "liboqs not found at ${LIBOQS_LIBRARY}. "
        "Build the main project first: cd ${AUTOSAR_ROOT} && bash rebuild_pqc.sh")
endif()

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Collect Autosar_SecOC C sources we need (reuse, no duplication).
# We do NOT pull main.c – the ISE provides its own entry-point.
set(AUTOSAR_SOURCES
    ${AUTOSAR_ROOT}/source/PQC/PQC.c
    ${AUTOSAR_ROOT}/source/PQC/PQC_KeyDerivation.c
    ${AUTOSAR_ROOT}/source/PQC/PQC_KeyExchange.c
)

# Simulator framework (our own code).
set(ISE_FRAMEWORK
    src/sim_clock.c
    src/sim_logger.c
    src/sim_metrics.c
    src/sim_bus.c
    src/sim_signals.c
    src/sim_attacker.c
    src/sim_ecu.c
)

# Scenario library.
set(ISE_SCENARIOS
    scenarios/sc_common.c
    scenarios/sc_baseline.c
    scenarios/sc_throughput.c
    scenarios/sc_mixed_bus.c
    scenarios/sc_attacks.c
    scenarios/sc_rekey.c
    scenarios/sc_persistence.c
    scenarios/sc_bus_failure.c
    scenarios/sc_deadline_stress.c
    scenarios/sc_multi_ecu.c
    scenarios/sc_keymismatch.c
    scenarios/sc_rollover.c
    scenarios/sc_freshness_overflow.c
    scenarios/sc_replay_boundary.c
    scenarios/sc_verify_disabled.c
    scenarios/sc_flexray_baseline.c
)

add_executable(ise_runner
    scenarios/scenario_runner.c
    ${ISE_SCENARIOS}
    ${ISE_FRAMEWORK}
    ${AUTOSAR_SOURCES}
)

target_include_directories(ise_runner PRIVATE
    include
    ${AUTOSAR_ROOT}/include
    ${AUTOSAR_ROOT}/include/PQC
    ${AUTOSAR_ROOT}/include/Com
    ${LIBOQS_INCLUDE_DIR}
)

target_link_libraries(ise_runner PRIVATE
    ${LIBOQS_LIBRARY}
    OpenSSL::Crypto
    OpenSSL::SSL
    Threads::Threads
    m
)

target_compile_definitions(ise_runner PRIVATE
    ISE_INTEGRATED_SIMULATION=1
    _GNU_SOURCE=1
)

if(UNIX)
    target_compile_definitions(ise_runner PRIVATE LINUX=1)
elseif(WIN32)
    target_compile_definitions(ise_runner PRIVATE WINDOWS=1)
    target_link_libraries(ise_runner PRIVATE ws2_32)
endif()

#
# CTest integration — make the ISE runnable via `ctest` too.
#
include(CTest)
enable_testing()
add_test(NAME ise_baseline
         COMMAND ise_runner --scenario baseline --iterations 50
                            --protection pqc --out "${CMAKE_BINARY_DIR}/ctest_out/baseline")
add_test(NAME ise_attacks
         COMMAND ise_runner --scenario attacks  --iterations 40
                            --protection pqc --out "${CMAKE_BINARY_DIR}/ctest_out/attacks")
add_test(NAME ise_rekey
         COMMAND ise_runner --scenario rekey    --iterations 50
                            --out "${CMAKE_BINARY_DIR}/ctest_out/rekey")
add_test(NAME ise_persistence
         COMMAND ise_runner --scenario persistence --iterations 40
                            --protection pqc
                            --out "${CMAKE_BINARY_DIR}/ctest_out/persistence")
add_test(NAME ise_bus_failure
         COMMAND ise_runner --scenario bus_failure --iterations 40
                            --protection hmac
                            --out "${CMAKE_BINARY_DIR}/ctest_out/bus_failure")
add_test(NAME ise_deadline_stress
         COMMAND ise_runner --scenario deadline_stress --iterations 30
                            --protection pqc
                            --out "${CMAKE_BINARY_DIR}/ctest_out/deadline_stress")
add_test(NAME ise_multi_ecu
         COMMAND ise_runner --scenario multi_ecu --iterations 30
                            --protection pqc
                            --out "${CMAKE_BINARY_DIR}/ctest_out/multi_ecu")
add_test(NAME ise_keymismatch
         COMMAND ise_runner --scenario keymismatch --iterations 40
                            --protection pqc
                            --out "${CMAKE_BINARY_DIR}/ctest_out/keymismatch")
add_test(NAME ise_rollover
         COMMAND ise_runner --scenario rollover --iterations 20
                            --protection pqc
                            --out "${CMAKE_BINARY_DIR}/ctest_out/rollover")
add_test(NAME ise_freshness_overflow
         COMMAND ise_runner --scenario freshness_overflow --iterations 20
                            --protection pqc
                            --out "${CMAKE_BINARY_DIR}/ctest_out/freshness_overflow")
add_test(NAME ise_replay_boundary
         COMMAND ise_runner --scenario replay_boundary --iterations 20
                            --protection pqc
                            --out "${CMAKE_BINARY_DIR}/ctest_out/replay_boundary")
add_test(NAME ise_verify_disabled
         COMMAND ise_runner --scenario verify_disabled --iterations 50
                            --out "${CMAKE_BINARY_DIR}/ctest_out/verify_disabled")
add_test(NAME ise_flexray_baseline
         COMMAND ise_runner --scenario flexray_baseline --iterations 30
                            --protection pqc
                            --out "${CMAKE_BINARY_DIR}/ctest_out/flexray_baseline")

message(STATUS "ISE: ise_runner target configured")
message(STATUS "ISE: AUTOSAR_ROOT = ${AUTOSAR_ROOT}")
message(STATUS "ISE: liboqs      = ${LIBOQS_LIBRARY}")
