set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)

set(CMAKE_C_COMPILER emcc)
set(CMAKE_CXX_COMPILER em++)

set(LINKED_LIBS
    "-s USE_GLFW=3"
    "-s FULL_ES3=1"
    "-s ALLOW_MEMORY_GROWTH=1"
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --no-heap-copy")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s WASM=1 -s FORCE_FILESYSTEM=1 -s FETCH=1 --no-heap-copy --emrun -o house_of_atmos.html")

set(CMAKE_EXECUTABLE_SUFFIX ".html")