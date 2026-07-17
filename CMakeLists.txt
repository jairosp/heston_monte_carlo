cmake_minimum_required(VERSION 3.25)
project(HestonMonteCarlo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Python Graphs!
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# GTest
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
)
FetchContent_MakeAvailable(googletest)

# I have compiled this as library so it does not have to compile multiple times
add_library(heston_core STATIC src/heston.cpp)
target_include_directories(heston_core PUBLIC include)

# MAIN
add_executable(heston_sim src/main.cpp)
target_link_libraries(heston_sim PRIVATE heston_core)

# BENCHMARKS
add_executable(heston_benchmarks benchmarks/benchmarks.cpp)
target_link_libraries(heston_benchmarks PRIVATE heston_core)
target_compile_definitions(heston_benchmarks PRIVATE 
    PROJECT_ROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

# TESTING
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
set(COMPILER_WARNINGS
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
)

# HPC
foreach(target heston_core heston_sim heston_benchmarks heston_tests)
    target_compile_options(${target} PRIVATE ${COMPILER_WARNINGS})
    
    target_compile_options(${target} PRIVATE 
        $<$<CONFIG:Release>:-O3 -march=native -ffast-math>
    )
endforeach()

set(VENV_DIR "${CMAKE_BINARY_DIR}/venv")

# PYTHON LIBRARIES FOR SCRIPTING
if(WIN32)
    set(VENV_PYTHON "${VENV_DIR}/Scripts/python.exe")
    set(VENV_PIP "${VENV_DIR}/Scripts/pip.exe")
else()
    set(VENV_PYTHON "${VENV_DIR}/bin/python")
    set(VENV_PIP "${VENV_DIR}/bin/pip")
endif()

if(NOT EXISTS "${VENV_DIR}")
    message(STATUS "Creando entorno virtual de Python en: ${VENV_DIR}")
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -m venv ${VENV_DIR}
        RESULT_VARIABLE venv_create_result
    )
    if(NOT venv_create_result EQUAL 0)
        message(FATAL_ERROR "No se pudo crear el entorno virtual de Python.")
    endif()
endif()

message(STATUS "Validando dependencias de Python (pandas, matplotlib, numpy)...")
execute_process(
    COMMAND ${VENV_PIP} install -r "${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt"
    RESULT_VARIABLE pip_install_result
)
if(NOT pip_install_result EQUAL 0)
    message(FATAL_ERROR "Error al instalar las dependencias de Python desde requirements.txt")
endif()

# BENCHMARKS PIPELINE
add_custom_target(run_benchmarks
    COMMAND $<TARGET_FILE:heston_benchmarks>
    COMMAND ${VENV_PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/plot_benchmarks.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Benchmarking C++ code..."
    DEPENDS heston_benchmarks
)