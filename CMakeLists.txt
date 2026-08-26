cmake_minimum_required(VERSION 3.10)
project(RubiksCube)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(RubiksCube src/main.cpp src/cube.cpp)

if(EMSCRIPTEN)
    set_target_properties(RubiksCube PROPERTIES SUFFIX ".html")
    target_compile_options(RubiksCube PUBLIC "-sUSE_SDL=2")
    target_link_options(RubiksCube PUBLIC "-sUSE_SDL=2" "-sFULL_ES2=1" "--shell-file" "${CMAKE_CURRENT_SOURCE_DIR}/shell.html" "-sEXPORTED_RUNTIME_METHODS=ccall,cwrap")
else()
    find_package(SDL2 REQUIRED)
    find_package(OpenGL REQUIRED)
    include_directories(${SDL2_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR})
    target_link_libraries(RubiksCube ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES})
endif()
