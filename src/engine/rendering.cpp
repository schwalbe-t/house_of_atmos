
#include "engine.hpp"
#include <cstring>

namespace houseofatmos::engine::rendering {

    static Color* initialize_buffer(int width, int height) {
        Color* data = new Color[width * height];
        for(int y = 0; y < height; y += 1) {
            for(int x = 0; x < width; x += 1) {
                data[y * width + x] = BLACK;
            }
        }
        return data;
    }

    FrameBuffer::FrameBuffer(int width, int height) {
        this->width = width;
        this->height = height;
        this->data = initialize_buffer(width, height);
    }

    FrameBuffer::~FrameBuffer() {
        delete this->data;
    }

    Color FrameBuffer::get_pixel_at(int x, int y) {
        return this->data[y * this->width + x];
    }

    void FrameBuffer::set_pixel_at(int x, int y, Color c) {
        this->data[y * this->width + x] = c;
    }

    void FrameBuffer::resize(int new_width, int new_height) {
        if(new_width == this->width && new_height == this->height) {
            return;
        }
        delete this->data;
        this->width = new_width;
        this->height = new_height;
        this->data = initialize_buffer(new_width, new_height);
    }

}