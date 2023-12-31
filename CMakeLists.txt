cmake_minimum_required(VERSION 3.10)

project(ParallelComputationLabs C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)

# SDL setup to disable these annoying tests & unused shared lib
set(SDL_TEST_ENABLED_BY_DEFAULT OFF)
set(SDL_SHARED_ENABLED_BY_DEFAULT OFF)

set(SDL2_PATH "${CMAKE_SOURCE_DIR}/extern/SDL2")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

add_subdirectory(${SDL2_PATH})

set(SDL2_TARGET SDL2-static)
find_path(SDL2_INCLUDE_DIRS NAMES SDL.h PATHS ${SDL2_PATH}/include)

add_subdirectory(${CMAKE_SOURCE_DIR}/common)

add_subdirectory("${CMAKE_SOURCE_DIR}/Lab 1")
add_subdirectory("${CMAKE_SOURCE_DIR}/Lab 2")
add_subdirectory("${CMAKE_SOURCE_DIR}/Lab 3")
add_subdirectory("${CMAKE_SOURCE_DIR}/Lab 4")
add_subdirectory("${CMAKE_SOURCE_DIR}/Lab 5")
add_subdirectory("${CMAKE_SOURCE_DIR}/Lab 6")
