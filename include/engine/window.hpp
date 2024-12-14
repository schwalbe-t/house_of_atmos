
#pragma once

#include "rendering.hpp"
#include "scene.hpp"
#include <memory>

namespace houseofatmos::engine {

    struct Window {

        private:
        void* ptr;
        i32 last_width;
        i32 last_height;
        f64 last_time;
        f64 frame_delta;

        std::shared_ptr<Scene> current_scene;


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

        void set_scene(std::shared_ptr<Scene> scene);
        std::shared_ptr<Scene> scene(); 
        void start();

        void show_texture(const Texture& texture) const;

    };

}