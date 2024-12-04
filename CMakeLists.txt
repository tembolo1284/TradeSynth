cmake_minimum_required(VERSION 3.10)
project(TradeSynth C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pthread")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -DNDEBUG")

# Find required packages
find_package(Threads REQUIRED)
find_package(CUnit REQUIRED)

# Set directories
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(TEST_DIR ${CMAKE_SOURCE_DIR}/tests)

# Include directories
include_directories(${INCLUDE_DIR})

# Source files
file(GLOB SERVER_SOURCES "${SRC_DIR}/server/*.c")
file(GLOB CLIENT_SOURCES "${SRC_DIR}/client/*.c")
file(GLOB COMMON_SOURCES "${SRC_DIR}/common/*.c")
file(GLOB SERIAL_SOURCES "${SRC_DIR}/serialization/*.c")
file(GLOB TEST_SOURCES "${TEST_DIR}/unit/*.c" "${TEST_DIR}/integration/*.c")
file(GLOB MOCK_SOURCES "${TEST_DIR}/mock/*.c")

# Create library for common code
add_library(common_lib STATIC
    ${COMMON_SOURCES}
    ${SERIAL_SOURCES}
)

# Server executable
add_executable(trading_server ${SERVER_SOURCES})
target_link_libraries(trading_server
    common_lib
    Threads::Threads
)

# Client executable
add_executable(trading_client ${CLIENT_SOURCES})
target_link_libraries(trading_client
    common_lib
    Threads::Threads
)

# Tests executable
add_executable(run_tests
    ${TEST_SOURCES}
    ${MOCK_SOURCES}
)
target_link_libraries(run_tests
    common_lib
    Threads::Threads
    cunit
)

# Install targets
install(TARGETS trading_server trading_client
    RUNTIME DESTINATION bin
)

# Enable testing
enable_testing()
add_test(NAME unit_tests COMMAND run_tests)

# Custom targets
add_custom_target(test_verbose
    COMMAND ${CMAKE_BINARY_DIR}/run_tests
    DEPENDS run_tests
)
