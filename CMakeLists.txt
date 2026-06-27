cmake_minimum_required(VERSION 3.25)
project(HestonMonteCarlo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include the headers directory
include_directories(include)

# Define the executable and its source files
add_executable(heston_sim 
    src/main.cpp 
    src/heston.cpp
)