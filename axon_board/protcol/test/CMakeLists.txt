cmake_minimum_required(VERSION 3.16)

project(Catch2UnitTests LANGUAGES CXX C)

# Set C standard
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define source directories
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src)

# Create the test executable
add_executable(UnitTest01
    UnitTest01.cpp
    ${SRC_DIR}/target.c
)

# Include directories
target_include_directories(UnitTest01 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${SRC_DIR}
)

# Compiler-specific settings
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(STATUS "Using Clang compiler")

    # Clang-specific flags
    target_compile_options(UnitTest01 PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(STATUS "Using MSVC compiler")

    # MSVC-specific flags
    target_compile_options(UnitTest01 PRIVATE
        /W4
        /permissive-
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Using GCC compiler")

    # GCC-specific flags
    target_compile_options(UnitTest01 PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# Enable testing
enable_testing()
add_test(NAME UnitTest01 COMMAND UnitTest01)
