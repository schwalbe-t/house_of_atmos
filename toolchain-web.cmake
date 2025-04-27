set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)

set(CMAKE_C_COMPILER emcc)
set(CMAKE_CXX_COMPILER em++)

set(LINKED_LIBS
    "-s USE_GLFW=3"
    "-s FULL_ES3=1"
)
set(LINKER_OPTIONS
    "-s USE_GLFW=3"
    "-sFULL_ES3"
    "-sUSE_WEBGL2"
    "-sALLOW_MEMORY_GROWTH"
    "-sWASM"
    "-sFORCE_FILESYSTEM"
    "--preload-file" "${CMAKE_SOURCE_DIR}/res@/res"
    "-sLZ4"
    "-s MODULARIZE=1"
    "-s EXPORT_NAME=house_of_atmos"
    "-s STANDALONE_WASM=0"
    "--shell-file" "web-shell.html"
    "-o house_of_atmos.html"
    "--emrun"
    "--no-embed-file"
    "--no-pointer-lock"
)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --no-heap-copy")
set(OUT_NAME "house_of_atmos.html")