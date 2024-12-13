
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

    static void center_window(GLFWwindow* window, i32 width, i32 height) {
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
        i32 window_x = (mode->width - width) / 2;
        i32 window_y = (mode->height - height) / 2;
        glfwSetWindowPos(window, window_x, window_y);        
    }

    Window::Window(i32 width, i32 height, const char* name) {
        if(width <= 0 || height <= 0) {
            error("Window width and height must both be larger than 0"
                " (given was " + std::to_string(width)
                + "x" + std::to_string(height) + ")"
            );
        }
        if(existing_windows == 0) { init_gltf(); }
        this->ptr = (GLFWwindow*) glfwCreateWindow(
            width, height, name, NULL, NULL
        );
        if(this->ptr == nullptr) {
            glfwTerminate();
            error("Unable to initialize the window!");
        }
        this->last_width = width;
        this->last_height= height;
        center_window((GLFWwindow*) this->ptr, width, height);
        glfwMakeContextCurrent((GLFWwindow*) this->ptr);
        gladLoadGL(&glfwGetProcAddress);
        glfwSwapInterval(0);
        glEnable(GL_DEPTH_TEST);
        existing_windows += 1;
    }

    Window::~Window() {
        glfwDestroyWindow((GLFWwindow*) this->ptr);
        existing_windows -= 1;
        if(existing_windows == 0) {
            glfwTerminate();
        }
    }


    bool Window::next_frame() {
        glfwSwapBuffers((GLFWwindow*) this->ptr);
        glfwPollEvents();
        glfwGetFramebufferSize(
            (GLFWwindow*) this->ptr, &this->last_width, &this->last_height
        );
        f64 current_time = glfwGetTime();
        this->frame_delta = current_time - this->last_time;
        this->last_time = current_time;
        return !glfwWindowShouldClose((GLFWwindow*) this->ptr);
    }

    i32 Window::width() const { return this->last_width; }
    i32 Window::height() const { return this->last_height; }
    f64 Window::delta_time() const { return this->frame_delta; }


    void Window::show_texture(const Texture& texture) {
        f64 scale = (f64) this->height() / texture.height();
        f64 dest_width = texture.width() * scale;
        i64 dest_x = (this->width() - dest_width) / 2;
        texture.internal_blit(
            0, this->width(), this->height(),
            dest_x, 0, dest_width, this->height()
        );
    }

}