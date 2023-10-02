cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)

project(rlib LANGUAGES C CXX VERSION 1.0.0)

include(../cmake/Utils.cmake)
include(CMakePackageConfigHelpers)

option(RLIB_BUILD_CORE "Build rlib core" ON)
option(RLIB_BUILD_GRAPHICS "Build rlib graphics" ON)
option(RLIB_BUILD_WRAPPERS "Build rlib wrappers" ON)

if(RLIB_BUILD_CORE)
    add_subdirectory(${PROJECT_SOURCE_DIR}/core)
endif()

if(RLIB_BUILD_GRAPHICS)
    add_subdirectory(${PROJECT_SOURCE_DIR}/graphics)
endif()

if(RLIB_BUILD_WRAPPERS)
    add_subdirectory(${PROJECT_SOURCE_DIR}/wrappers)
endif()

