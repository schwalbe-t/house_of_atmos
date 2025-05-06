
set(CMAKE_C_COMPILER cc)
set(CMAKE_CXX_COMPILER c++)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGL_GLEXT_PROTOTYPES -DGL_ES_VERSION_3_0")

set(GLES2_LIBRARIES /usr/lib64/libGLESv2.so)
set(EGL_LIBRARIES /usr/lib64/libEGL.so)

set(REQUIRED_PACKAGES
    OpenGL
    glfw3
    OpenAL
)

set(LINKED_LIBS
    ${GLES2_LIBRARIES}
    ${EGL_LIBRARIES}
    glfw
    OpenAL::OpenAL
)
set(LINKER_OPTIONS "")
set(OUT_NAME "house_of_atmos")