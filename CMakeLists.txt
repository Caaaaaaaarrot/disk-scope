cmake_minimum_required(VERSION 3.10)

project(DiskScope)

set(CMAKE_CXX_STANDARD 17)

add_executable(disk_scope
    src/main.cpp
    src/analyzer.cpp
    src/utils.cpp
)

target_include_directories(disk_scope PRIVATE include)