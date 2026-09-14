cmake_minimum_required(VERSION 3.10)
project(LOB_sim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include fairPriceModels analysis)

# Source files
file(GLOB SOURCES "src/*.cpp")

# Create library
add_library(lob_lib ${SOURCES})

# Market-making agents and fair-price models
file(GLOB MM_SOURCES "agents/*.cpp" "fairPriceModels/*.cpp")
add_library(mm_lib ${MM_SOURCES})
target_link_libraries(mm_lib lob_lib)

# Tests
add_executable(test_price_level tests/testPriceLevel.cpp)
target_link_libraries(test_price_level lob_lib)

add_executable(test_order_node tests/testOrderNode.cpp)
target_link_libraries(test_order_node lob_lib)

add_executable(test_lob tests/testLimitOrderBook.cpp)
target_link_libraries(test_lob lob_lib)

# Analysis / benchmark programs backing the price-impact and complexity
# claims -- see analysis/*.cpp for what each one measures.
add_executable(price_impact_study
  analysis/PriceImpactStudy.cpp
  analysis/OrderFlowSimulator.cpp)
target_link_libraries(price_impact_study lob_lib)

add_executable(complexity_benchmark analysis/ComplexityBenchmark.cpp)
target_link_libraries(complexity_benchmark lob_lib)

add_executable(heap_vs_tree_benchmark analysis/HeapVsTreeBenchmark.cpp)
target_link_libraries(heap_vs_tree_benchmark lob_lib)
