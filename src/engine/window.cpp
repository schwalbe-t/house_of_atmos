
#include <engine/window.hpp>
#include <engine/logging.hpp>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

namespace houseofatmos::engine {
    
    static u64 existing_windows = 0;

    static void init_gltf() {
        glfwSetErrorCallback(&internal::glfw_error);
        if(!glfwInit()) {
            error("Unable to initialize the window!");
        }
    }

    static void center_window(GLFWwindow* window, u32 width, u32 height) {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        if(monitor == NULL) {
            warning("Unable to center the window!");
            return;
        }
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        if(mode == NULL) {
            warning("Unable to center the window!");
            return;
        }
        u32 window_x = (mode->width - width) / 2;
        u32 window_y = (mode->height - height) / 2;
        glfwSetWindowPos(window, window_x, window_y);        
    }

    Window::Window(u32 width, u32 height, const char* name, bool vsync) {
        if(existing_windows == 0) { init_gltf(); }
        this->ptr = (GLFWwindow*) glfwCreateWindow(
            width, height, name, NULL, NULL
        );
        if(this->ptr == nullptr) {
            glfwTerminate();
            error("Unable to initialize the window!");
        }
        center_window((GLFWwindow*) this->ptr, width, height);
        glfwMakeContextCurrent((GLFWwindow*) this->ptr);
        gladLoadGL(&glfwGetProcAddress);
        glfwSwapInterval(vsync? 1 : 0);
        existing_windows += 1;
    }

    bool Window::is_open() {
        glfwPollEvents();
        return !glfwWindowShouldClose((GLFWwindow*) this->ptr);
    }

    void* Window::internal_ptr() {
        return this->ptr;
    }

    Window::~Window() {
        glfwDestroyWindow((GLFWwindow*) this->ptr);
        existing_windows -= 1;
        if(existing_windows == 0) {
            glfwTerminate();
        }
    }

}