cmake_minimum_required(VERSION 3.2)
project(helloworld)
set(CMAKE_CXX_STANDARD 11)

option(ENABLE_COVERAGE "Enable coverage reporting" OFF)

if(ENABLE_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")

    # Setup target for running lcov to capture coverage data
    add_custom_target(coverage
        COMMAND lcov --directory ${CMAKE_BINARY_DIR} --zerocounters
        COMMAND ${PROJECT_NAME}-Runner # Run your test executable
        COMMAND lcov --directory ${CMAKE_BINARY_DIR} --capture --output-file coverage.info
        COMMAND lcov --remove coverage.info '/usr/*' --output-file coverage.info # filtering system-files
        COMMAND genhtml coverage.info --output-directory coverage_output # generate HTML
        COMMENT "Generating coverage report in 'coverage_output/'")
endif()

# Main executable
add_executable(${PROJECT_NAME} helloworld.cpp PrimeChecker.cpp)





enable_testing()
# Test executable
add_executable(${PROJECT_NAME}-Runner TestPrimeChecker.cpp PrimeChecker.cpp)
add_test(NAME ${PROJECT_NAME}-Runner COMMAND ${PROJECT_NAME}-Runner)


