cmake_minimum_required(VERSION 3.20)
project(ConcurrentMemoryPool VERSION 1.0.0 LANGUAGES CXX)

option(CMP_BUILD_TESTS "Build the test suite" ON)
option(CMP_BUILD_BENCHMARKS "Build benchmark example" ON)

add_library(concurrent_memory_pool INTERFACE)
add_library(cmp::concurrent_memory_pool ALIAS concurrent_memory_pool)
target_include_directories(concurrent_memory_pool INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_compile_features(concurrent_memory_pool INTERFACE cxx_std_20)

if(CMP_BUILD_TESTS)
  enable_testing()
  add_executable(cmp_tests tests/test_memory_pool.cpp)
  target_link_libraries(cmp_tests PRIVATE cmp::concurrent_memory_pool)
  add_test(NAME cmp_tests COMMAND cmp_tests)
endif()

if(CMP_BUILD_BENCHMARKS)
  add_executable(cmp_benchmark examples/benchmark.cpp)
  target_link_libraries(cmp_benchmark PRIVATE cmp::concurrent_memory_pool)
endif()
