
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
        this->last_height = height;
        this->last_time = 0;
        this->frame_delta = 0;
        this->current_scene = nullptr;
        this->next_scene = nullptr;
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


    i32 Window::width() const { return this->last_width; }
    i32 Window::height() const { return this->last_height; }
    f64 Window::delta_time() const { return this->frame_delta; }


    void Window::set_scene(std::shared_ptr<Scene> scene) {
        this->next_scene = scene;
    }

    std::shared_ptr<Scene> Window::scene() {
        return this->current_scene;
    }

    void Window::start() {
        while(!glfwWindowShouldClose((GLFWwindow*) this->ptr)) {
            glfwPollEvents();
            glfwGetFramebufferSize(
                (GLFWwindow*) this->ptr, &this->last_width, &this->last_height
            );
            f64 current_time = glfwGetTime();
            this->frame_delta = current_time - this->last_time;
            this->last_time = current_time;
            if(this->current_scene) {
                this->current_scene->update(*this);
                this->current_scene->render(*this);
            }
            if(this->next_scene) {
                if(this->current_scene) {
                    this->current_scene->internal_forget_all();
                }
                this->current_scene = this->next_scene;
                this->current_scene->internal_load_all();
                this->next_scene = nullptr;
            }
            glfwSwapBuffers((GLFWwindow*) this->ptr);
        }
    }


    void Window::show_texture(const Texture& texture) const {
        f64 scale = (f64) this->height() / texture.height();
        f64 dest_width = texture.width() * scale;
        i64 dest_x = (this->width() - dest_width) / 2;
        texture.internal_blit(
            0, this->width(), this->height(),
            dest_x, 0, dest_width, this->height()
        );
    }

}