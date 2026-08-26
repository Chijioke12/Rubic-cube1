cmake_minimum_required(VERSION 3.10)
project(RubiksCube)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(RubiksCube src/main.cpp src/cube.cpp)

if(EMSCRIPTEN)
    set_target_properties(RubiksCube PROPERTIES SUFFIX ".html")
    target_link_libraries(RubiksCube "-s USE_SDL=2")
else()
    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIRS})
    target_link_libraries(RubiksCube ${SDL2_LIBRARIES})
endif()
