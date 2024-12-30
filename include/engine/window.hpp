
#pragma once

#include "rendering.hpp"
#include "scene.hpp"
#include "input.hpp"
#include <memory>

namespace houseofatmos::engine {

    struct Window {

        private:
        const static inline size_t key_array_length
            = (size_t) Key::MaximumValue + 1;
        const static inline size_t button_array_length
            = (size_t) Button::MaximumValue + 1;
        
        void* ptr;
        i32 last_width;
        i32 last_height;
        f64 last_time;
        f64 frame_delta;

        std::shared_ptr<Scene> current_scene;
        std::shared_ptr<Scene> next_scene;

        std::array<bool, key_array_length> keys_down_curr = {};
        std::array<bool, key_array_length> keys_down_last = {};
        Vec<2> mouse_pos;
        std::array<bool, button_array_length> buttons_down_curr = {};
        std::array<bool, button_array_length> buttons_down_last = {};

        static void glfw_key_callback(
            void* glfw_window_raw, 
            int key, int scancode, int action, int mods
        );
        static void glfw_cursor_pos_callback(
            void* glfw_window_raw, f64 x, f64 y
        );
        static void glfw_mouse_button_callback(
            void* glfw_window_raw, 
            int button, int action, int mods
        );
        void update_last_frame_input();


        public:
        Window(i32 width, i32 height, const char* name);
        Window(const Window& other) = delete;
        Window(Window&& other) = delete;
        Window& operator=(const Window& other) = delete;
        Window& operator=(Window&& other) = delete;
        ~Window();

        i32 width() const;
        i32 height() const;
        f64 delta_time() const;
        f64 time() const;

        bool is_down(Key key) const {
            return this->keys_down_curr[(size_t) key]; 
        }
        bool was_pressed(Key key) const {
            return !this->keys_down_last[(size_t) key]
                && this->keys_down_curr[(size_t) key];
        }
        bool was_released(Key key) const {
            return !this->keys_down_curr[(size_t) key]
                && this->keys_down_last[(size_t) key];
        }
        
        const Vec<2>& cursor_pos_px() const { return this->mouse_pos; }
        const Vec<2> cursor_pos_ndc() const {
            Vec<2> normalized = this->mouse_pos
                / Vec<2>(this->width(), this->height());
            return Vec<2>(normalized.x(), 1 - normalized.y()) * 2
                - Vec<2>(1.0, 1.0);
        }
        void show_cursor() const;
        void hide_cursor() const;
        bool is_down(Button button) const {
            return this->buttons_down_curr[(size_t) button]; 
        }
        bool was_pressed(Button button) const {
            return !this->buttons_down_last[(size_t) button]
                && this->buttons_down_curr[(size_t) button];
        }
        bool was_released(Button button) const {
            return !this->buttons_down_curr[(size_t) button]
                && this->buttons_down_last[(size_t) button];
        }

        void set_scene(std::shared_ptr<Scene> scene);
        std::shared_ptr<Scene> scene(); 
        void start();

        void show_texture(const Texture& texture) const;

    };

}