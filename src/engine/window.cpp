
#include <engine/window.hpp>
#include <engine/logging.hpp>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <AL/alc.h>

namespace houseofatmos::engine {
    
    static std::unordered_map<GLFWwindow*, Window*> existing_windows;

    static void init_gltf() {
        glfwSetErrorCallback(&internal::glfw_error);
        if(!glfwInit()) {
            error("Unable to initialize the window!");
        }
    }
    static void free_gltf() {
        glfwTerminate();
    }

    static ALCdevice* audio_device = nullptr;
    static ALCcontext* audio_context = nullptr;

    static void init_openal() {
        audio_device = alcOpenDevice(nullptr);
        if(audio_device == nullptr) {
            error("Unable to open audio device");
        }
        audio_context = alcCreateContext(audio_device, nullptr);
        if(!alcMakeContextCurrent(audio_context)) {
            error("Failed to make the OpenAL context the current");
        }
    }
    static void free_openal() {
        if(audio_device != nullptr) {
            alcCloseDevice(audio_device);
        }
        if(audio_context != nullptr) {
            alcDestroyContext(audio_context);
        }
    }

    static void init_opengl() {
        gladLoadGL(&glfwGetProcAddress);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
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

    void Window::glfw_key_callback(
        void* glfw_window_raw, int key, int scancode, int action, int mods
    ) {
        (void) scancode;
        (void) mods;
        GLFWwindow* glfw_window = (GLFWwindow*) glfw_window_raw;
        if(!existing_windows.contains(glfw_window)) { return; }
        Window* window = existing_windows[glfw_window];
        if(action == GLFW_PRESS) {
            window->keys_down_curr[key] = true;
        }   
        if(action == GLFW_RELEASE) {
            window->keys_down_curr[key] = false;
        }
    }

    void Window::glfw_cursor_pos_callback(
        void* glfw_window_raw, f64 x, f64 y
    ) {
        GLFWwindow* glfw_window = (GLFWwindow*) glfw_window_raw;
        if(!existing_windows.contains(glfw_window)) { return; }
        Window* window = existing_windows[glfw_window];
        window->mouse_pos = { x, y };
    }

    void Window::glfw_mouse_button_callback(
        void* glfw_window_raw, int button, int action, int mods
    ) {
        (void) mods;
        GLFWwindow* glfw_window = (GLFWwindow*) glfw_window_raw;
        if(!existing_windows.contains(glfw_window)) { return; }
        Window* window = existing_windows[glfw_window];
        window->buttons_down_curr[button] = action == GLFW_PRESS;
    }

    void Window::glfw_scroll_callback(
        void* glfw_window_raw, f64 x, f64 y
    ) {
        GLFWwindow* glfw_window = (GLFWwindow*) glfw_window_raw;
        if(!existing_windows.contains(glfw_window)) { return; }
        Window* window = existing_windows[glfw_window];
        window->scroll_dist.x() += x;
        window->scroll_dist.y() += y;
    }

    void Window::update_last_frame_input() {
        std::memcpy(
            (void*) this->keys_down_last.data(), 
            (void*) this->keys_down_curr.data(), 
            sizeof(bool) * key_array_length
        );
        std::memcpy(
            (void*) this->buttons_down_last.data(),
            (void*) this->buttons_down_curr.data(), 
            sizeof(bool) * button_array_length
        );
        this->scroll_dist = Vec<2>();
    }


    Window::Window(
        u64 width, u64 height, const std::string& name, 
        std::optional<Image> icon
    ) {
        if(width == 0 || height == 0) {
            error("Window width and height must both be larger than 0"
                " (given was " + std::to_string(width)
                + "x" + std::to_string(height) + ")"
            );
        }
        if(existing_windows.size() == 0) {
            init_gltf();
            init_openal();
        }
        this->ptr = (GLFWwindow*) glfwCreateWindow(
            width, height, name.c_str(), NULL, NULL
        );
        if(this->ptr == nullptr) {
            glfwTerminate();
            error("Unable to initialize the window!");
        }
        this->original_width = width;
        this->original_height = height;
        this->fullscreen = false;
        this->last_width = width;
        this->last_height = height;
        this->last_time = 0;
        this->frame_delta = 0;
        this->current_scene = nullptr;
        this->next_scene = nullptr;
        if(icon.has_value()) {
            GLFWimage icon_img;
            icon_img.width = icon->width();
            icon_img.height = icon->height();
            icon_img.pixels = (unsigned char*) icon->data();
            glfwSetWindowIcon((GLFWwindow*) this->ptr, 1, &icon_img);
        }
        center_window((GLFWwindow*) this->ptr, width, height);
        glfwMakeContextCurrent((GLFWwindow*) this->ptr);
        glfwSwapInterval(0);
        init_opengl();
        glfwSetKeyCallback(
            (GLFWwindow*) this->ptr, 
            (GLFWkeyfun) &Window::glfw_key_callback
        );
        glfwSetCursorPosCallback(
            (GLFWwindow*) this->ptr, 
            (GLFWcursorposfun) &Window::glfw_cursor_pos_callback
        );
        glfwSetMouseButtonCallback(
            (GLFWwindow*) this->ptr, 
            (GLFWmousebuttonfun) &Window::glfw_mouse_button_callback
        );
        glfwSetScrollCallback(
            (GLFWwindow*) this->ptr,
            (GLFWscrollfun) &Window::glfw_scroll_callback
        );
        existing_windows[(GLFWwindow*) this->ptr] = this;
    }

    Window::~Window() {
        glfwDestroyWindow((GLFWwindow*) this->ptr);
        existing_windows.erase((GLFWwindow*) this->ptr);
        if(existing_windows.size() == 0) {
            free_gltf();
            free_openal();
        }
    }


    u64 Window::width() const { return this->last_width; }
    u64 Window::height() const { return this->last_height; }
    Vec<2> Window::size() const {
        return Vec<2>(this->width(), this->height()); 
    }
    f64 Window::delta_time() const { return this->frame_delta; }
    f64 Window::time() const { return this->last_time; }


    void Window::show_cursor() const {
        GLFWwindow* window = (GLFWwindow*) this->ptr;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    void Window::hide_cursor() const {
        GLFWwindow* window = (GLFWwindow*) this->ptr;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }


    void Window::set_windowed() {
        if(!this->is_fullscreen()) { return; }
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        if(monitor == NULL) {
            warning("Unable to make the window windowed!");
            return;
        }
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        if(mode == NULL) {
            warning("Unable to make the window windowed!");
            return;
        }
        i32 x = (mode->width - this->original_width) / 2;
        i32 y = (mode->height - this->original_height) / 2;
        glfwSetWindowMonitor(
            (GLFWwindow*) this->ptr, NULL,
            x, y, 
            this->original_width, this->original_height, 
            0
        );
        this->fullscreen = false;
    }

    void Window::set_fullscreen() {
        if(this->is_fullscreen()) { return; }
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        if(monitor == NULL) {
            warning("Unable to make the window fullscreen!");
            return;
        }
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        if(mode == NULL) {
            warning("Unable to make the window fullscreen!");
            return;
        }
        glfwSetWindowMonitor(
            (GLFWwindow*) this->ptr, monitor,
            0, 0, mode->width, mode->height, mode->refreshRate
        );
        this->fullscreen = true;
    }


    void Window::set_scene(std::shared_ptr<Scene>&& scene) {
        this->next_scene = std::move(scene);
    }

    std::shared_ptr<Scene> Window::scene() {
        return this->current_scene;
    }

    void Window::start() {
        while(!glfwWindowShouldClose((GLFWwindow*) this->ptr)) {
            this->update_last_frame_input();
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
                this->current_scene = this->next_scene;
                this->current_scene->internal_load_all();
                Scene::clean_cached_resources();
                this->next_scene = nullptr;
            }
            glfwSwapBuffers((GLFWwindow*) this->ptr);
        }
    }


    void Window::show_texture(const Texture& texture) const {
        f64 scale = (f64) this->height() / texture.height();
        f64 dest_width = texture.width() * scale;
        i64 dest_x = (this->width() - dest_width) / 2;
        texture.blit(
            (RenderTarget) { 0, this->width(), this->height() },
            dest_x, 0, dest_width, this->height()
        );
    }

}