cmake_minimum_required(VERSION 3.26)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_ROOT}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_ROOT}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_ROOT}/bin)

project(Graphics2023 LANGUAGES C CXX VERSION 0.1)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic -Wno-gnu-anonymous-struct -Wno-pedantic -Wno-nested-anon-types -Wno-unused-function)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-Wno-unused-variable -Wno-unused-parameter -Wno-pedantic -Wno-unused-but-set-variable)
    endif()
endif()

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

add_subdirectory(thirdparty)
add_subdirectory(workspace)