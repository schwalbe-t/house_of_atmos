
cmake_minimum_required(VERSION 3.6.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(OpenGL_GL_PREFERENCE GLVND)

project(
    house_of_atmos
    VERSION 0.1
    DESCRIPTION "A strategy game about turning a small county into an economical powerhouse."
    LANGUAGES CXX
)

# Comment these out when cross-compiling to Windows >:^)
# find_package(OpenGL REQUIRED)
# find_package(glfw3 3.3 REQUIRED)
# find_package(OpenAL REQUIRED)

file(GLOB_RECURSE LIB_SOURCES "libsrc/*.cpp")
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(
    house_of_atmos
    ${LIB_SOURCES}
    ${SOURCES}
)

include_directories("include")
include_directories("glad/include")

# Comment the first out when cross-compiling to Windows >:^)
# target_link_libraries(
#     house_of_atmos PRIVATE
#     OpenGL::GL
#     OpenGL::GLU
#     glfw
#     OpenAL::OpenAL
# )
target_link_libraries(
    house_of_atmos PRIVATE
    ${GLFW_LIBRARIES}
    ${OpenAL_LIBRARIES}
)

target_compile_features(house_of_atmos PRIVATE cxx_std_20)
# target_compile_options(house_of_atmos PRIVATE -g -Wall -Wextra -O0)
target_compile_options(house_of_atmos PRIVATE -Wall -Wextra -O3 -static-libgcc -static-libstdc++)