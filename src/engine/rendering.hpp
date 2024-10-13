
#pragma once

#include <raylib.h>
#include <vector>
#include <tuple>
#include <cstdint>
#include "math.hpp"

namespace houseofatmos::engine::rendering {

    using namespace houseofatmos::engine::math;

    static double triangle_area(Vec<2> a, Vec<2> b, Vec<2> c) {
        return 0.5 * fabs(
            a.x() * (b.y() - c.y())
                + b.x() * (c.y() - a.y())
                + c.x() * (a.y() - b.y())
        );
    }

    template<typename V>
    struct Mesh {
        std::vector<V> vertices;
        std::vector<std::tuple<uint32_t, uint32_t, uint32_t>> elements;

        Mesh() {}
        
        Mesh(const Mesh&) = delete;
        Mesh(Mesh&& other) {
            this->vertices = std::move(other.vertices);
            this->elements = std::move(other.elements);
        }

        Mesh& operator=(const Mesh& other) = delete;
        Mesh& operator=(Mesh&& other) noexcept {
            this->vertices = std::move(other.vertices);
            this->elements = std::move(other.elements);
            return *this;
        }

        uint32_t add_vertex(V vertex) {
            uint32_t idx = this->vertices.size();
            this->vertices.push_back(vertex);
            return idx;
        }

        void add_element(uint32_t a, uint32_t b, uint32_t c) {
            this->elements.push_back(std::make_tuple(a, b, c));
        }
    };

    template<typename V, typename S>
    struct VertexStates {
        S a_state; // shader after running 'vertex' for A
        double a_idepth; // inverse of depth of A
        double a_bc; // barycentric coordinate for A
        S b_state; // shader after running 'vertex' for B
        double b_idepth ; // inverse of depth of B
        double b_bc; // barycentric coordinate for B
        S c_state; // shader after running 'vertex' for C
        double c_idepth; // inverse of depth of C
        double c_bc; // barycentric coordinate for C
        double idepth; // current inverse depth
    };

    template<typename V, typename S>
    struct Shader {
        virtual Vec<4> vertex(V vertex) = 0;
        virtual Vec<4> fragment() = 0;

        private:
        const VertexStates<V, S>* vertex_states;

        public:
        void set_vertex_states(const VertexStates<V, S>* states) {
            this->vertex_states = states;
        }

        void clear_vertex_states() {
            this->vertex_states = nullptr;
        }

        template<typename T>
        void interpolate(T* value) {
            // check that pointers are correct
            if(this->vertex_states == nullptr) {
                std::cout << "'interpolate' can only be called from 'fragment'!" 
                    << std::endl;
                std::abort();
            }
            int64_t offset = (char*) value - (char*) this;
            if(offset < 0 || (size_t) offset > sizeof(S)) {
                std::cout << "Interpolated value must be a shader property!" 
                    << std::endl;
                std::abort();
            }
            const VertexStates<V, S>* vs = this->vertex_states;
            // get values for all three vertices
            const T* a = (const T*) ((char*) &vs->a_state + offset);
            const T* b = (const T*) ((char*) &vs->b_state + offset);
            const T* c = (const T*) ((char*) &vs->c_state + offset);
            // interpolate them using the barycentric coordinates
            *value = (
                (*a * vs->a_idepth * vs->a_bc) + 
                (*b * vs->b_idepth * vs->b_bc) + 
                (*c * vs->c_idepth * vs->c_bc)
            ) / vs->idepth;
        }
    };

    struct Surface {
        int width;
        int height;
        Color* color;
        float* depth;

        Surface(int width, int height);
        Surface(const Color* color, const float* depth, int width, int height);
        Surface(const Surface&) = delete;
        Surface(Surface&&);
        Surface& operator=(const Surface& other) = delete;
        Surface& operator=(Surface&& other) noexcept;
        ~Surface();
        Color get_color_at(int x, int y) const;
        void set_color_at(int x, int y, Color c);
        double get_depth_at(int x, int y) const;
        void set_depth_at(int x, int y, double d);
        Vec<4> sample(const Vec<2>& uv) const;

        void clear();

        void blit_buffer(
            const Surface& src, 
            int dest_pos_x, int dest_pos_y,
            int dest_width, int dest_height
        );

        private: 
        template<typename V, typename S>
        void render_triangle_segment(
            Vec<3> a, Vec<3> b, Vec<3> c, double t_area,
            Vec<2> s_high, // top vertex of the segment
            Vec<2> s_low, // bottom vertex of the segment
            VertexStates<V, S>* vs,
            S& shader
        ) {
            Vec<2> t_high = a.swizzle<2>("xy"); // top vertex of the triangle
            Vec<2> t_low = c.swizzle<2>("xy"); // bottom vertex of the triangle
            Vec<2> s_line = s_low - s_high; // vector from top to bottom of segment
            Vec<2> t_line = t_low - t_high; // vector from top to bottom of triangle
            for(int y = std::max((int) s_high.y() + 1, 0); y < s_low.y(); y += 1) {
                if(y > this->height) { break; }
                double s_progress = (y - s_high.y()) / s_line.y();
                Vec<2> r_point = s_line * s_progress + s_high;
                double t_progress = (y - t_high.y()) / t_line.y();
                Vec<2> l_point = t_line * t_progress + t_high;
                if(l_point.x() > r_point.x()) { std::swap(l_point, r_point); }
                for(int x = std::max((int) l_point.x() + 1, 0); x <= r_point.x(); x += 1) {
                    if(x > this->width) { break; }
                    Vec<2> p = Vec<2>(x, y);
                    vs->a_bc = triangle_area(p, b.swizzle<2>("xy"), c.swizzle<2>("xy")) / t_area;
                    vs->b_bc = triangle_area(p, c.swizzle<2>("xy"), a.swizzle<2>("xy")) / t_area;
                    vs->c_bc = triangle_area(p, a.swizzle<2>("xy"), b.swizzle<2>("xy")) / t_area;
                    vs->idepth = vs->a_bc * vs->a_idepth + vs->b_bc * vs->b_idepth
                        + vs->c_bc * vs->c_idepth;
                    double depth = 1.0 / vs->idepth;
                    if(depth >= this->get_depth_at(x, y)) {
                        continue;
                    }
                    Color color = shader.fragment().as_color();
                    this->set_color_at(x, y, color);
                    this->set_depth_at(x, y, depth);
                }
            }
        }

        template<typename V, typename S>
        void draw_triangle(V vertex_a, V vertex_b, V vertex_c, S& shader) {
            VertexStates<V, S> vs;
            // get positions from vertex shader
            vs.a_state = shader;
            Vec<4> a_ndc = vs.a_state.vertex(vertex_a);
            vs.a_idepth = 1.0 / a_ndc.z();
            vs.b_state = shader;
            Vec<4> b_ndc = vs.b_state.vertex(vertex_b);
            vs.b_idepth = 1.0 / b_ndc.z();
            vs.c_state = shader;
            Vec<4> c_ndc = vs.c_state.vertex(vertex_c);
            vs.c_idepth = 1.0 / c_ndc.z();
            // convert vertices to pixel space
            Mat<4> to_pixel_space
                = Mat<4>::translate(Vec<2>(this->width, this->height))
                * Mat<4>::scale(Vec<2>(this->width / 2 * -1, this->height / 2 * -1))
                * Mat<4>::translate(Vec<2>(1.0, 1.0));
            Vec<3> a = (to_pixel_space * (a_ndc / a_ndc.w())).swizzle<3>("xyz");
            Vec<3> b = (to_pixel_space * (b_ndc / b_ndc.w())).swizzle<3>("xyz");
            Vec<3> c = (to_pixel_space * (c_ndc / c_ndc.w())).swizzle<3>("xyz");
            // sort vertex pixel positions (a, b, c in order of ascending y)
            if(a.y() > b.y()) {
                std::swap(a, b);
                std::swap(vs.a_state, vs.b_state);
                std::swap(vs.a_idepth, vs.b_idepth);
            } 
            if(b.y() > c.y()) { 
                std::swap(b, c);
                std::swap(vs.b_state, vs.c_state);
                std::swap(vs.b_idepth, vs.c_idepth);
            } 
            if(a.y() > b.y()) { 
                std::swap(a, b); 
                std::swap(vs.a_state, vs.b_state);
                std::swap(vs.a_idepth, vs.b_idepth);
            }
            // compute size of triangle
            double t_area = triangle_area(
                a.swizzle<2>("xy"), b.swizzle<2>("xy"), c.swizzle<2>("xy")
            );
            // draw traingle segments
            shader.set_vertex_states(&vs);
            // segment: high -> mid (top half)
            render_triangle_segment(
                a, b, c, t_area,
                a.swizzle<2>("xy"), b.swizzle<2>("xy"), &vs, shader
            );
            // segment: mid -> low (bottom half)
            render_triangle_segment(
                a, b, c, t_area,
                b.swizzle<2>("xy"), c.swizzle<2>("xy"), &vs, shader
            );
            shader.clear_vertex_states();
        }

        public:
        template<typename V, typename S>
        void draw_mesh(const Mesh<V>& mesh, S& shader) {
            static_assert(std::is_base_of<Shader<V, S>, S>(), "Must be a shader!");
            static_assert(std::is_copy_constructible<S>(), "Must be copyable!");
            for(uint16_t elem_i = 0; elem_i < mesh.elements.size(); elem_i += 1) {
                auto indices = mesh.elements[elem_i];
                this->draw_triangle(
                    mesh.vertices[std::get<0>(indices)],
                    mesh.vertices[std::get<1>(indices)],
                    mesh.vertices[std::get<2>(indices)],
                    shader
                );
            } 
        }

    };

}