
#pragma once

#include <raylib.h>
#include <vector>
#include <tuple>
#include <cstdint>
#include "math.hpp"

namespace houseofatmos::engine::rendering {

    using namespace houseofatmos::engine::math;

    template<typename V>
    struct Mesh {
        std::vector<V> vertices;
        std::vector<std::tuple<uint16_t, uint16_t, uint16_t>> elements;

        Mesh() {}
        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh& other) = delete;
        Mesh& operator=(Mesh&& other) noexcept {
            this->vertices = std::move(other.vertices);
            this->elements = std::move(other.elements);
            return *this;
        }

        uint16_t add_vertex(V vertex) {
            uint16_t idx = this->vertices.size();
            this->vertices.push_back(vertex);
            return idx;
        }

        void add_element(uint16_t a, uint16_t b, uint16_t c) {
            this->elements.push_back(std::make_tuple(a, b, c));
        }
    };

    template<typename V>
    struct Shader {
        virtual void vertex(V vertex, Vec<4>* pos) = 0;
        virtual void fragment(Color* color) = 0;
    };

    struct FrameBuffer {
        int width;
        int height;
        Color* data;

        FrameBuffer(int width, int height);
        FrameBuffer(const FrameBuffer&) = delete;
        FrameBuffer& operator=(const FrameBuffer& other) = delete;
        FrameBuffer& operator=(FrameBuffer&& other) noexcept;
        ~FrameBuffer();
        Color get_pixel_at(int x, int y) const;
        Color get_pixel_at(Vec<2> location) const;
        void set_pixel_at(int x, int y, Color c);
        void set_pixel_at(Vec<2> location, Color c);

        void clear();

        void blit_buffer(
            const FrameBuffer& src, 
            int dest_pos_x, int dest_pos_y,
            int dest_width, int dest_height
        );

        private: 
        template<typename V>
        void render_triangle_segment(
            Vec<2> t_high, // top vertex of the triangle
            Vec<2> t_low,  // bottom vertex of the triangle
            Vec<2> s_high, // top vertex of the segment
            Vec<2> s_low, // bottom vertex of the segment
            Shader<V>& shader
        ) {
            Vec<2> s_line = s_low - s_high; // vector from top to bottom of segment
            Vec<2> t_line = t_low - t_high; // vector from top to bottom of triangle
            for(int y = s_high.y() + 1; y < s_low.y(); y += 1) {
                if(y < 0) { y = 0; }
                if(y > this->height) { break; }
                double s_progress = (y - s_high.y()) / s_line.y();
                Vec<2> r_point = s_line * s_progress + s_high;
                double t_progress = (y - t_high.y()) / t_line.y();
                Vec<2> l_point = t_line * t_progress + t_high;
                if(l_point.x() > r_point.x()) { std::swap(l_point, r_point); }
                for(int x = l_point.x(); x < r_point.x(); x += 1) {
                    if(x < 0) { x = 0; }
                    if(x > this->width) { break; }
                    Color color = PURPLE;
                    shader.fragment(&color);
                    this->set_pixel_at(x, y, color);
                }
            }
        }

        template<typename V>
        void draw_rectangle(V a, V b, V c, Shader<V>& shader) {
            // get positions from vertex shader
            Vec<4> a_ndc, b_ndc, c_ndc;
            shader.vertex(a, &a_ndc);
            shader.vertex(b, &b_ndc);
            shader.vertex(c, &c_ndc);
            // convert vertices to pixel space
            Mat<4> to_pixel_space
                = Mat<4>::translate(Vec<2>(this->width, this->height))
                * Mat<4>::scale(Vec<2>(this->width / 2 * -1, this->height / 2 * -1))
                * Mat<4>::translate(Vec<2>(1.0, 1.0));
            Vec<2> high = (to_pixel_space * (a_ndc / a_ndc.w())).swizzle<2>("xy"); 
            Vec<2> mid = (to_pixel_space * (b_ndc / b_ndc.w())).swizzle<2>("xy");
            Vec<2> low = (to_pixel_space * (c_ndc / c_ndc.w())).swizzle<2>("xy");
            // sort vertex pixel positions by y position
            if(high.y() > mid.y()) { std::swap(high, mid); } 
            if(mid.y() > low.y()) { std::swap(mid, low); } 
            if(high.y() > mid.y()) { std::swap(high, mid); }
            // segment: high -> mid (top half)
            render_triangle_segment(high, low, high, mid, shader);
            // segment: mid -> low (bottom half)
            render_triangle_segment(high, low, mid, low, shader);
        }

        public:
        template<typename V>
        void draw_mesh(const Mesh<V>& mesh, Shader<V>& shader) {
            for(uint16_t elem_i = 0; elem_i < mesh.elements.size(); elem_i += 1) {
                auto indices = mesh.elements[elem_i];
                this->draw_rectangle(
                    mesh.vertices[std::get<0>(indices)],
                    mesh.vertices[std::get<1>(indices)],
                    mesh.vertices[std::get<2>(indices)],
                    shader
                );
            } 
        }

    };

}