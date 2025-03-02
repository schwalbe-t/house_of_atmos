
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(REQUIRED_PACKAGES
    OpenGL
    glfw3
    OpenAL
)

set(LINKED_LIBS
    OpenGL::GL
    OpenGL::GLU
    glfw
    OpenAL::OpenAL
)
