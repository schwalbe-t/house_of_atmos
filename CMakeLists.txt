
cmake_minimum_required(VERSION 3.6.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)
set(OpenGL_GL_PREFERENCE GLVND)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/toolchain-default.cmake" CACHE FILEPATH "Default toolchain file")
endif()

project(
    house_of_atmos
    VERSION 0.1
    DESCRIPTION "A strategy game about turning a small county into an economical powerhouse."
    LANGUAGES CXX C
)

foreach(PKG ${REQUIRED_PACKAGES})
    find_package(${PKG} REQUIRED)
endforeach()

file(GLOB_RECURSE LIB_SOURCES "libsrc/*.cpp" "libsrc/*.c")
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")
add_executable(
    house_of_atmos
    ${LIB_SOURCES}
    ${SOURCES}
)

set_target_properties(house_of_atmos PROPERTIES LINKER_LANGUAGE CXX)

include_directories("include")
include_directories("glad/include")

target_link_libraries(
    house_of_atmos PRIVATE
    ${LINKED_LIBS}
)

target_compile_features(house_of_atmos PRIVATE cxx_std_20)

set(MODE "debug" CACHE STRING "Build mode: debug or release")
string(TOLOWER "${MODE}" MODE)
message(MODE="${MODE}")
if(MODE STREQUAL "debug")
    target_compile_options(house_of_atmos PRIVATE -g -Wall -Wextra -O0)
elseif(MODE STREQUAL "release")
    target_compile_options(house_of_atmos PRIVATE -Wall -Wextra -O3)
else()
    message(FATAL_ERROR "Unknown build mode '${MODE}'")
endif()
