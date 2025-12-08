cmake_minimum_required(VERSION 3.10)
project(LOB_sim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directory
include_directories(include)

# Source files
file(GLOB SOURCES "src/*.cpp")

# Create library
add_library(lob_lib ${SOURCES})

# Tests
add_executable(test_price_level tests/testPriceLevel.cpp)
target_link_libraries(test_price_level lob_lib)

add_executable(test_order_node tests/testOrderNode.cpp)
target_link_libraries(test_order_node lob_lib)

add_executable(test_lob tests/testLimitOrderBook.cpp)
target_link_libraries(test_lob lob_lib)
