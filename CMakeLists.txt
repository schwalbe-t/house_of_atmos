
cmake_minimum_required(VERSION 3.6.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(
    house_of_atmos
    VERSION 0.1
    DESCRIPTION "A completely software-rendered 3D game."
    LANGUAGES CXX
)

find_package(raylib REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(
    house_of_atmos
    ${SOURCES}
)

target_link_libraries(house_of_atmos raylib)

target_compile_features(house_of_atmos PRIVATE cxx_std_20)
target_compile_options(house_of_atmos PRIVATE -Wall -Wextra -Wpedantic -g)
