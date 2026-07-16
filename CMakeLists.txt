cmake_minimum_required(VERSION 3.25)
project(HestonMonteCarlo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include the headers directory
include_directories(include)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
)

# This line solved the bug of not recognising gtest
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
    heston_tests 
    tests/test_convergence.cc 
    tests/test_properties.cc
    tests/test_rng.cc
    src/heston.cpp
)

target_link_libraries(
  heston_tests
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(heston_tests)

add_executable(
    heston_sim 
    src/main.cpp 
    src/heston.cpp
)