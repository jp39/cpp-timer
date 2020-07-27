project(cpp-timer)
cmake_minimum_required(VERSION 3.16)

option(CPPTIMER_BUILD_TESTS "Build cpp-timer tests" OFF)

add_library(cpptimer INTERFACE)
target_include_directories(cpptimer INTERFACE ${CMAKE_CURRENT_LIST_DIR})
find_library(LIBRT rt)
if(LIBRT)
  target_link_libraries(cpptimer INTERFACE ${LIBRT})
endif()

if(CPPTIMER_BUILD_TESTS)
  add_executable(timer_test test/main.cpp)
  target_link_libraries(timer_test cpptimer)
  set_property(TARGET timer_test PROPERTY CXX_STANDARD 11)
endif()
