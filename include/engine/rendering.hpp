
#pragma once

#include "math.hpp"
#include <vector>
#include <span>

namespace houseofatmos::engine {

    struct Shader {

        private:
        u64 vert_id;
        u64 frag_id;
        u64 prog_id;
        bool moved;


        public:
        Shader(const char* vertex, const char* fragment);
        Shader(const Shader& other) = delete;
        Shader(Shader&& other) noexcept;
        Shader& operator=(const Shader& other) = delete;
        Shader& operator=(Shader&& other) noexcept;
        ~Shader();

        void internal_bind() const;
        void internal_unbind() const;

    };


    struct Texture {

        private:
        u64 width_px;
        u64 height_px;

        u64 fbo_id;
        u64 tex_id;
        u64 dbo_id;
        bool moved;


        public:
        Texture(u64 width, u64 height);
        Texture(const Texture& other) = delete;
        Texture(Texture&& other) noexcept;
        Texture& operator=(const Texture& other) = delete;
        Texture& operator=(Texture&& other) noexcept;
        ~Texture();

        u64 width() const;
        u64 height() const;

        void internal_bind_frame() const;
        void internal_unbind_frame() const;
        u64 internal_fbo_id() const;
        u64 internal_tex_id() const;

        void clear_color(Vec<4> color) const;
        void clear_depth(f64 depth) const;

        void resize_fast(u64 width, u64 height);
        void resize(u64 width, u64 height);

    };


    struct Mesh {

        private:
        std::vector<u8> attrib_sizes;
        u16 vertex_size;
        std::vector<f32> vertices;
        std::vector<u16> elements;
        
        u64 vbo_id;
        u64 ebo_id;
        u64 buff_index_count;
        bool modified;
        bool moved;

        void init_buffers();
        void bind_properties() const;
        void unbind_properties() const;
        void delete_buffers() const;


        public:
        Mesh(std::initializer_list<u8> attrib_sizes);
        Mesh(const Mesh& other) = delete;
        Mesh(Mesh&& other) noexcept;
        Mesh& operator=(const Mesh& other) = delete;
        Mesh& operator=(Mesh&& other) noexcept;
        ~Mesh();

        u16 add_vertex(std::span<f32> data);
        u16 add_vertex(std::initializer_list<f32> data);
        u16 vertex_count() const;
        void add_element(u16 a, u16 b, u16 c);
        u32 element_count() const;
        void clear();

        void submit();
        void render(const Shader& shader, const Texture& dest);

    };

}