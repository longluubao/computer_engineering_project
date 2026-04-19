cmake_minimum_required(VERSION 3.20)

set(This SecOCTests)

file(GLOB_RECURSE Sources CONFIGURE_DEPENDS "*.cpp")

# MCAL Pi4 driver functions (Dio_*, Mcu_*, Gpt_*, Wdg_*) only exist when
# building with MCAL_TARGET=PI4. McalTests.cpp references them directly, so
# when the parent configured a SIM build those symbols are absent and the
# test fails to link. Skip that test binary in SIM mode.
if(NOT MCAL_TARGET STREQUAL "PI4")
    list(FILTER Sources EXCLUDE REGEX ".*/McalTests\\.cpp$")
endif()

foreach (_file ${Sources})

    cmake_path(GET _file STEM executable)
    add_executable(${executable} ${_file})
    target_link_libraries(${executable} PUBLIC
        gtest_main
        SecOCLib
    )

    # Output JUnit XML results for Cantata-style report generation
    add_test(
        NAME ${executable}
        COMMAND ${executable} --gtest_output=xml:${CMAKE_BINARY_DIR}/test_results/${executable}.xml
    )

endforeach()
