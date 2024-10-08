
#pragma once

#include <raylib.h>
#include "math.hpp"

namespace houseofatmos::engine::rendering {

    using namespace houseofatmos::engine::math;

    struct FrameBuffer {
        int width;
        int height;
        Color* data;

        FrameBuffer(int width, int height);
        FrameBuffer(const FrameBuffer&) = delete;
        FrameBuffer& operator=(const FrameBuffer& other) = delete;
        FrameBuffer& operator=(FrameBuffer&& other);
        ~FrameBuffer();
        Color get_pixel_at(int x, int y);
        Color get_pixel_at(Vec<2> location);
        void set_pixel_at(int x, int y, Color c);
        void set_pixel_at(Vec<2> location, Color c);

        void clear();

        void draw_line(Vec<2> a, Vec<2> b, Color color);
        void draw_triangle(Vec<2> a, Vec<2> b, Vec<2> c, Color color);
    };

}

namespace houseofatmos::engine {

    void init(const char* title, int width, int height, int fps);
    bool is_running();
    void display_buffer(rendering::FrameBuffer* buffer);
    void stop();

}