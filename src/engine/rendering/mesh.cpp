
#include <engine/rendering.hpp>
#include <engine/logging.hpp>
#include <glad/gl.h>
#include <numeric>

namespace houseofatmos::engine {

    Mesh::Mesh(std::initializer_list<u8> attrib_sizes) {
        this->attrib_sizes = std::vector(attrib_sizes);
        this->vertex_size = std::reduce(attrib_sizes.begin(), attrib_sizes.end());
        this->init_buffers();
    }

    u16 Mesh::add_vertex(std::span<f32> data) {
        if(data.size() != this->vertex_size) {
            error("Attempted to push a vertex with "
                + std::to_string(data.size())
                + " onto a mesh with vertices of size "
                + std::to_string(this->vertex_size)
                + " (sizes must match)"
            );
        }
        u16 index = this->vertex_count();
        this->vertices.insert(this->vertices.end(), data.begin(), data.end());
        return index;
    }

    u16 Mesh::add_vertex(std::initializer_list<f32> data) {
        if(data.size() != this->vertex_size) {
            error("Attempted to push a vertex with "
                + std::to_string(data.size())
                + " onto a mesh with vertices of size "
                + std::to_string(this->vertex_size)
                + " (sizes must match)"
            );
        }
        u16 index = this->vertex_count();
        this->vertices.insert(this->vertices.end(), data.begin(), data.end());
        return index;
    }

    u16 Mesh::vertex_count() {
        return this->vertices.size() / this->vertex_size;
    }

    void Mesh::add_element(u16 a, u16 b, u16 c) {
        this->elements.push_back(a);
        this->elements.push_back(b);
        this->elements.push_back(c);
    }

    u32 Mesh::element_count() {
        return this->elements.size() / 3;
    }

    void Mesh::clear() {
        this->vertices.clear();
        this->elements.clear();
        this->buff_index_count = 0;
    }


    void Mesh::init_buffers() {
        GLuint current_gid;
        glGenBuffers(1, &current_gid);
        this->vert_id = current_gid;
        glGenBuffers(1, &current_gid);
        this->elem_id = current_gid;
        glGenVertexArrays(1, &current_gid);
        this->varr_id = current_gid;
    }

    void Mesh::init_property_pointers() {
        glBindBuffer(GL_ARRAY_BUFFER, this->vert_id);
        glBindVertexArray(this->varr_id);
        u64 vertex_size = this->vertex_size * sizeof(f32);
        u64 offset = 0;
        for(u64 attr = 0; attr < this->attrib_sizes.size(); attr += 1) {
            u8 attr_size = this->attrib_sizes[attr];
            glEnableVertexAttribArray(attr);
            glVertexAttribPointer(
                attr, attr_size, GL_FLOAT, GL_FALSE, vertex_size, (void*) offset
            );
            offset += attr_size * sizeof(f32);
        }
        glBindVertexArray(GL_NONE);
        glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);
    }

    void Mesh::submit() {
        if(this->vertices.size() == 0 || this->elements.size() == 0) {
            return;
        }
        glBindVertexArray(this->varr_id);
        glBindBuffer(GL_ARRAY_BUFFER, this->vert_id);
        glBufferData(
            GL_ARRAY_BUFFER,
            this->vertices.size() * sizeof(f32),
            this->vertices.data(),
            GL_STATIC_DRAW
        );
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->elem_id);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            this->elements.size() * sizeof(u16),
            this->elements.data(),
            GL_STATIC_DRAW
        );
        glBindVertexArray(GL_NONE);
        this->buff_index_count = this->elements.size();
    }

    Mesh::~Mesh() {
        GLuint current_gid;
        current_gid = this->vert_id;
        glDeleteBuffers(1, &current_gid);
        current_gid = this->elem_id;
        glDeleteBuffers(1, &current_gid);
        current_gid = this->varr_id;
        glDeleteVertexArrays(1, &current_gid);
    }

    u64 Mesh::internal_vert_id() { return this->vert_id; }
    u64 Mesh::internal_elem_id() { return this->elem_id; }
    u64 Mesh::internal_varr_id() { return this->varr_id; }
    u64 Mesh::internal_buffered_indices() { return this->buff_index_count; }

}