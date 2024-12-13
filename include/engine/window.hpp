
#pragma once

#include "rendering.hpp"

namespace houseofatmos::engine {

    struct Window {

        private:
        void* ptr;
        i32 last_width;
        i32 last_height;
        f64 last_time;
        f64 frame_delta;


        public:
        Window(i32 width, i32 height, const char* name);
        Window(const Window& other) = delete;
        Window& operator=(const Window& other) = delete;
        ~Window();

        bool next_frame();
        i32 width() const;
        i32 height() const;
        f64 delta_time() const;

        void show_texture(const Texture& texture);

    };

}