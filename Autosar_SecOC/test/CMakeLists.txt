cmake_minimum_required(VERSION 3.20)

set(This SecOCTests)

file(GLOB_RECURSE Sources CONFIGURE_DEPENDS "*.cpp")

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
