
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
        this->last_width = width;
        this->last_height= height;
        center_window((GLFWwindow*) this->ptr, width, height);
        glfwMakeContextCurrent((GLFWwindow*) this->ptr);
        gladLoadGL(&glfwGetProcAddress);
        glfwSwapInterval(vsync? 1 : 0);
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

    u32 Window::width() const { return this->last_width; }
    u32 Window::height() const { return this->last_height; }
    f64 Window::delta_time() const { return this->frame_delta; }


    void Window::show_texture(const Texture& texture) {
        f64 scale = (f64) this->height() / texture.height();
        f64 dest_width = texture.width() * scale;
        i64 dest_x = (this->width() - dest_width) / 2;
        glBindFramebuffer(GL_READ_FRAMEBUFFER, texture.internal_fbo_id());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(
            0,      0, texture.width(),     texture.height(),
            dest_x, 0, dest_x + dest_width, this->height(),
            GL_COLOR_BUFFER_BIT, GL_NEAREST
        );
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

}