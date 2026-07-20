cmake_minimum_required(VERSION 3.25)
project(HestonMonteCarlo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# CUDA CONFIGURATION SETTINGS
option(ENABLE_CUDA "Configure with CUDA acceleration scheme" OFF)

set(CORE_SOURCES
    src/cpu/cpu_heston_pricer.cpp
)

if(ENABLE_CUDA)
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    
    # In case CUDA is enabled, add it to the core library
    list(APPEND CORE_SOURCES src/cuda/cuda_heston_pricer.cu)
    add_compile_definitions(BUILD_CUDA)
endif()

# DEPENDENCIES, Python and GTest for testing and benchmarking
find_package(Python3 COMPONENTS Interpreter REQUIRED)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
)
FetchContent_MakeAvailable(googletest)

# CORE Library: Avoids creating the class with every executable
add_library(heston_core STATIC ${CORE_SOURCES}) # Core sources was defined to include CUDA or not
target_include_directories(heston_core PUBLIC include)

if(ENABLE_CUDA)
    set_target_properties(heston_core PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
    )
endif()

# MAIN SIMULATOR
add_executable(heston_sim src/main.cpp)
target_link_libraries(heston_sim PRIVATE heston_core)

# BENCHMARKS
add_executable(heston_benchmarks benchmarks/benchmarks.cpp)
target_link_libraries(heston_benchmarks PRIVATE heston_core)
target_compile_definitions(heston_benchmarks PRIVATE 
    PROJECT_ROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

# UNIT TESTING
enable_testing()
add_executable(
    heston_tests 
    tests/test_convergence.cc 
    tests/test_properties.cc
    tests/test_rng.cc
)
target_link_libraries(heston_tests PRIVATE heston_core GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(heston_tests)

# STRICT WARNINGS
set(CXX_WARNINGS
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
)

foreach(target heston_core heston_sim heston_benchmarks heston_tests)
    # NVCC ignores GCC flags
    target_compile_options(${target} PRIVATE 
        $<$<COMPILE_LANGUAGE:CXX>:${CXX_WARNINGS}>
    )
    
    # Configure flags depending on compiler
    target_compile_options(${target} PRIVATE 
        $<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CXX>>:-O3 -march=native -ffast-math>
        $<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CUDA>>:-O3 --use_fast_math>
    )
endforeach()

# Python config: Graphs, Venv etc...
set(VENV_DIR "${CMAKE_BINARY_DIR}/venv")

if(WIN32)
    set(VENV_PYTHON "${VENV_DIR}/Scripts/python.exe")
    set(VENV_PIP "${VENV_DIR}/Scripts/pip.exe")
else()
    set(VENV_PYTHON "${VENV_DIR}/bin/python")
    set(VENV_PIP "${VENV_DIR}/bin/pip")
endif()

if(NOT EXISTS "${VENV_DIR}")
    message(STATUS "Setting up python venv in: ${VENV_DIR}")
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -m venv ${VENV_DIR}
        RESULT_VARIABLE venv_create_result
    )
    if(NOT venv_create_result EQUAL 0)
        message(FATAL_ERROR "Python venv could not be created")
    endif()
endif()

message(STATUS "Validating dependencies (Numpy, Pandas, Matplotlib...)")
execute_process(
    COMMAND ${VENV_PIP} install -r "${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt"
    RESULT_VARIABLE pip_install_result
)
if(NOT pip_install_result EQUAL 0)
    message(FATAL_ERROR "Could not install python dependencies from requirements.txt")
endif()

# Benchmarking pipeline
add_custom_target(run_benchmarks
    COMMAND $<TARGET_FILE:heston_benchmarks>
    COMMAND ${VENV_PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/plot_benchmarks.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running C++ benchmarks and python graphs"
    DEPENDS heston_benchmarks
)