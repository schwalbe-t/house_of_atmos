
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

    FrameBuffer& FrameBuffer::operator=(FrameBuffer&& other) {
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
        if(x < 0 || x > this->width || y < 0 || y > this->width) {
            return BLACK;
        }
        return this->data[y * this->width + x];
    }
    Color FrameBuffer::get_pixel_at(Vec<2> location) const {
        return this->get_pixel_at(location.x(), location.y());
    }

    void FrameBuffer::set_pixel_at(int x, int y, Color c) {
        if(x < 0 || x > this->width || y < 0 || y > this->width) {
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

    void FrameBuffer::draw_line(Vec<2> a, Vec<2> b, Color color) {
        Vec<2> direction = b - a;
        int step_count = direction.abs().sum();
        Vec<2> step = direction * (1.0 / step_count);
        Vec<2> position = a;
        for(int step_taken = 0; step_taken < step_count; step_taken += 1) {
            this->set_pixel_at(position, color);
            position += step;
        }
    }

    static void render_triangle_segment(
        FrameBuffer* buffer,
        Vec<2> t_high, // top vertex of the triangle
        Vec<2> t_low,  // bottom vertex of the triangle
        Vec<2> s_high, // top vertex of the segment
        Vec<2> s_low, // bottom vertex of the segment
        Color color
    ) {
        Vec<2> s_line = s_low - s_high; // vector from top to bottom of segment
        Vec<2> t_line = t_low - t_high; // vector from top to bottom of triangle
        // buffer->set_pixel_at(s_high, RED);
        // buffer->set_pixel_at(s_low, RED);
        for(int y = s_high.y() + 1; y < s_low.y(); y += 1) {
            double s_progress = (y - s_high.y()) / s_line.y();
            Vec<2> r_point = s_line * s_progress + s_high;
            double t_progress = (y - t_high.y()) / t_line.y();
            Vec<2> l_point = t_line * t_progress + t_high;
            if(l_point.x() > r_point.x()) { std::swap(l_point, r_point); }
            for(int x = l_point.x(); x <= r_point.x(); x += 1) {
                buffer->set_pixel_at(x, y, color);
            }
        }
    }

    void FrameBuffer::draw_triangle(Vec<2> a, Vec<2> b, Vec<2> c, Color color) {
        // sort three points according to their y-coordinate
        Vec<2> high = a;
        Vec<2> mid = b;
        Vec<2> low = c;
        if(high.y() > mid.y()) { std::swap(high, mid); } 
        if(mid.y() > low.y()) { std::swap(mid, low); } 
        if(high.y() > mid.y()) { std::swap(high, mid); }
        // segment: high -> mid (top half)
        render_triangle_segment(this, high, low, high, mid, color);
        // segment: mid -> low (bottom half)
        render_triangle_segment(this, high, low, mid, low, color);
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
