
#pragma once

#include "nums.hpp"
#include <vector>
#include <span>

namespace houseofatmos::engine {

    struct Shader {

        private:
        u64 vert_id;
        u64 frag_id;
        u64 prog_id;


        public:
        Shader(const char* vertex, const char* fragment);
        Shader(const Shader& other) = delete;
        Shader& operator=(const Shader& other) = delete;
        ~Shader();

        u64 internal_prog_id();

    };

    struct Mesh {

        private:
        std::vector<u8> attrib_sizes;
        u16 vertex_size;
        std::vector<f32> vertices;
        std::vector<u16> elements;
        
        u64 vert_id;
        u64 elem_id;
        u64 varr_id;
        u64 buff_index_count;

        void init_buffers();
        void init_property_pointers();


        public:
        Mesh(std::initializer_list<u8> attrib_sizes);
        Mesh(const Mesh& other) = delete;
        Mesh& operator=(const Mesh& other) = delete;
        ~Mesh();

        u16 add_vertex(std::span<f32> data);
        u16 add_vertex(std::initializer_list<f32> data);
        u16 vertex_count();
        void add_element(u16 a, u16 b, u16 c);
        u32 element_count();
        void clear();

        void submit();
        u64 internal_vert_id();
        u64 internal_elem_id();
        u64 internal_varr_id();
        u64 internal_buffered_indices();

    };

}