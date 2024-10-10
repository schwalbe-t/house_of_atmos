
#include "engine.hpp"
#include <cstring>
#include <iostream>

using namespace houseofatmos::engine::math;

namespace houseofatmos::engine::rendering {

    FrameBuffer::FrameBuffer(int width, int height) {
        this->width = width;
        this->height = height;
        this->data = new Color[width * height];
        for(int y = 0; y < height; y += 1) {
            for(int x = 0; x < width; x += 1) {
                data[y * width + x] = BLACK;
            }
        }
    }

    FrameBuffer& FrameBuffer::operator=(FrameBuffer&& other) noexcept {
        if(this == &other) { return *this; }
        delete[] this->data;
        this->data = other.data;
        this->width = other.width;
        this->height = other.height;
        other.data = nullptr;
        other.width = 0;
        other.height = 0;
        return *this;
    }

    FrameBuffer::~FrameBuffer() {
        if(this->data == nullptr) { return; }
        delete[] this->data;
        this->data = nullptr;
    }

    Color FrameBuffer::get_pixel_at(int x, int y) const {
        if(x < 0 || x >= this->width || y < 0 || y >= this->height) {
            return BLACK;
        }
        return this->data[y * this->width + x];
    }
    Color FrameBuffer::get_pixel_at(Vec<2> location) const {
        return this->get_pixel_at(location.x(), location.y());
    }

    void FrameBuffer::set_pixel_at(int x, int y, Color c) {
        if(x < 0 || x >= this->width || y < 0 || y >= this->height) {
            return;
        }
        this->data[y * this->width + x] = c;
    }
    void FrameBuffer::set_pixel_at(Vec<2> location, Color c) {
        this->set_pixel_at(location.x(), location.y(), c);
    }

    void FrameBuffer::clear() {
        for(int i = 0; i < this->width * this->height; i += 1) {
            this->data[i] = BLACK;
        }
    }

    void FrameBuffer::blit_buffer(
        const FrameBuffer& src, 
        int dest_pos_x, int dest_pos_y,
        int dest_width, int dest_height
    ) {
        int dest_end_x = dest_pos_x + dest_width;
        int dest_end_y = dest_pos_y + dest_height;
        for(int dest_x = dest_pos_x; dest_x < dest_end_x; dest_x += 1) {
            for(int dest_y = dest_pos_y; dest_y < dest_end_y; dest_y += 1) {
                float perc_x = (float) (dest_x - dest_pos_x) / dest_width;
                float perc_y = (float) (dest_y - dest_pos_y) / dest_height;
                int src_x = (int) (perc_x * src.width);
                int src_y = (int) (perc_y * src.height);
                Color pixel = src.get_pixel_at(src_x, src_y);
                this->set_pixel_at(dest_x, dest_y, pixel);
            }
        }
    }

}
