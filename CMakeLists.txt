
cmake_minimum_required(VERSION 3.6.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(OpenGL_GL_PREFERENCE GLVND)

project(
    house_of_atmos
    VERSION 0.1
    DESCRIPTION "A completely software-rendered 3D game."
    LANGUAGES CXX
)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")
set_source_files_properties("glad/src/gl.c" PROPERTIES LANGUAGE CXX)
add_executable(
    house_of_atmos
    "glad/src/gl.c"
    ${SOURCES}
)

include_directories("include")
include_directories("glad/include")

target_link_libraries(
    house_of_atmos PRIVATE
    OpenGL::GL
    OpenGL::GLU
    glfw
)

target_compile_features(house_of_atmos PRIVATE cxx_std_20)
target_compile_options(house_of_atmos PRIVATE -g)