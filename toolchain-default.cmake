
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

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
