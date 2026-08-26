cmake_minimum_required(VERSION 3.11)
project(RubiksCube)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CUSTOMIZE_BUILD ON CACHE BOOL "" FORCE)
set(PLATFORM "Web" CACHE STRING "" FORCE)

FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
)
FetchContent_MakeAvailable(raylib)

add_executable(RubiksCube src/main.cpp)

if(EMSCRIPTEN)
    set_target_properties(RubiksCube PROPERTIES SUFFIX ".html")
    target_link_options(RubiksCube PUBLIC "--shell-file" "${CMAKE_CURRENT_SOURCE_DIR}/shell.html")
    target_link_options(RubiksCube PUBLIC "-sUSE_GLFW=3")
endif()

target_link_libraries(RubiksCube PRIVATE raylib)

