set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_VERSION 10)

set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)

set(CMAKE_FIND_ROOT_PATH ${CMAKE_SOURCE_DIR}/windows_libs)

include_directories("${CMAKE_SOURCE_DIR}/windows_libs/include")
set(GLFW_LIBRARIES "${CMAKE_SOURCE_DIR}/windows_libs/lib/libglfw3dll.a")
set(OpenAL_LIBRARIES "${CMAKE_SOURCE_DIR}/windows_libs/lib/libOpenAL32.dll.a")

set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_INSTALL_RPATH "$ORIGIN")

set(LINKED_LIBS
    ${GLFW_LIBRARIES}
    ${OpenAL_LIBRARIES}
)
set(LINKER_OPTIONS
    "-static"
    "-Wl,-subsystem,windows"
)
set(OUT_NAME "house_of_atmos.exe")