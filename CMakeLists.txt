cmake_minimum_required(VERSION 3.12)

project(
    labwork3
    VERSION 1.0
    DESCRIPTION "C++ Year 2025 Labwork3"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(lib)
add_subdirectory(bin)

enable_testing()
add_subdirectory(tests)
