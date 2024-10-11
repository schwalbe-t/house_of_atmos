
#include "engine.hpp"
#include <cstring>
#include <iostream>

using namespace houseofatmos::engine::math;

namespace houseofatmos::engine::rendering {

    Surface::Surface(int width, int height) {
        this->width = width;
        this->height = height;
        this->color = new Color[width * height];
        this->depth = new float[width * height];
        for(int y = 0; y < height; y += 1) {
            for(int x = 0; x < width; x += 1) {
                this->color[y * width + x] = BLACK;
                this->depth[y * width + x] = INFINITY;
            }
        }
    }

    Surface& Surface::operator=(Surface&& other) noexcept {
        if(this == &other) { return *this; }
        delete[] this->color;
        this->color = other.color;
        this->depth = other.depth;
        this->width = other.width;
        this->height = other.height;
        other.color = nullptr;
        other.depth = nullptr;
        other.width = 0;
        other.height = 0;
        return *this;
    }

    Surface::~Surface() {
        if(this->color != nullptr) {
            delete[] this->color;
            this->color = nullptr;
        }
        if(this->depth != nullptr) {
            delete[] this->depth;
            this->depth = nullptr;
        }
    }

    Color Surface::get_color_at(int x, int y) const {
        if(x < 0 || x >= this->width || y < 0 || y >= this->height) {
            return BLACK;
        }
        return this->color[y * this->width + x];
    }

    void Surface::set_color_at(int x, int y, Color c) {
        if(x < 0 || x >= this->width || y < 0 || y >= this->height) {
            return;
        }
        this->color[y * this->width + x] = c;
    }

    double Surface::get_depth_at(int x, int y) const {
        if(this->depth == nullptr) { 
            return INFINITY; 
        }
        if(x < 0 || x >= this->width || y < 0 || y >= this->height) {
            return INFINITY;
        }
        return this->depth[y * this->width + x];
    }

    void Surface::set_depth_at(int x, int y, double d) {
        if(this->depth == nullptr) { 
            return; 
        }
        if(x < 0 || x >= this->width || y < 0 || y >= this->height) {
            return;
        }
        this->depth[y * this->width + x] = d;
    }

    void Surface::clear() {
        for(int i = 0; i < this->width * this->height; i += 1) {
            this->color[i] = BLACK;
            if(this->depth != nullptr) {
                this->depth[i] = INFINITY;
            }
        }
    }

    void Surface::blit_buffer(
        const Surface& src, 
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
                Color pixel = src.get_color_at(src_x, src_y);
                this->set_color_at(dest_x, dest_y, pixel);
            }
        }
    }

}
