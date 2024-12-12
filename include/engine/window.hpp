
#pragma once

#include "rendering.hpp"

namespace houseofatmos::engine {

    struct Window {

        private:
        void* ptr;
        int last_width;
        int last_height;
        f64 last_time;
        f64 frame_delta;


        public:
        Window(u32 width, u32 height, const char* name, bool vsync);
        Window(const Window& other) = delete;
        Window& operator=(const Window& other) = delete;
        ~Window();

        bool next_frame();
        u32 width() const;
        u32 height() const;
        f64 delta_time() const;

        void show_texture(const Texture& texture);

    };

}