cmake_minimum_required(VERSION 3.16)
project(AutelScanner VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -O2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -fsanitize=address")

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries (for Raspberry Pi)
find_library(BCM2835_LIB bcm2835)
if(BCM2835_LIB)
    target_link_libraries(${PROJECT_NAME} ${BCM2835_LIB})
endif()

# Find required packages
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} Threads::Threads)

# Install target
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# Testing
enable_testing()
if(BUILD_TESTING)
    file(GLOB TEST_SOURCES "tests/*.cpp")
    foreach(test_source ${TEST_SOURCES})
        get_filename_component(test_name ${test_source} NAME_WE)
        add_executable(${test_name} ${test_source})
        target_link_libraries(${test_name} ${PROJECT_NAME})
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()
