
#include <engine/rendering.hpp>
#include <engine/logging.hpp>
#include <glad/gl.h>
#include <numeric>

namespace houseofatmos::engine {

    Mesh::Mesh(std::initializer_list<u8> attrib_sizes) {
        this->attrib_sizes = std::vector(attrib_sizes);
        this->vertex_size = std::reduce(attrib_sizes.begin(), attrib_sizes.end());
        this->init_buffers();
        this->modified = false;
        this->moved = false;
    }

    Mesh::Mesh(Mesh&& other) noexcept {
        if(other.moved) {
            error("Attempted to move an already moved 'Mesh'");
        }
        this->attrib_sizes = std::move(other.attrib_sizes);
        this->vertex_size = other.vertex_size;
        this->vertices = std::move(other.vertices);
        this->elements = std::move(other.elements);
        this->vbo_id = other.vbo_id;
        this->ebo_id = other.ebo_id;
        this->buff_index_count = other.buff_index_count;
        this->modified = other.modified;
        this->moved = false;
        other.moved = true;
    }

    Mesh& Mesh::operator=(Mesh&& other) noexcept {
        if(this == &other) { return *this; }
        if(other.moved) {
            error("Attempted to move an already moved 'Mesh'");
        }
        if(!this->moved) {
            this->delete_buffers();
        }
        this->attrib_sizes = std::move(other.attrib_sizes);
        this->vertex_size = other.vertex_size;
        this->vertices = std::move(other.vertices);
        this->elements = std::move(other.elements);
        this->vbo_id = other.vbo_id;
        this->ebo_id = other.ebo_id;
        this->buff_index_count = other.buff_index_count;
        this->modified = other.modified;
        this->moved = false;
        other.moved = true;
        return *this;
    }

    Mesh::~Mesh() {
        if(this->moved) { return; }
        this->delete_buffers();
    }


    u16 Mesh::add_vertex(std::span<f32> data) {
        if(this->moved) {
            error("Attempted to use a moved 'Mesh'");
        }
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
        this->modified = true;
        return index;
    }

    u16 Mesh::add_vertex(std::initializer_list<f32> data) {
        if(this->moved) {
            error("Attempted to use a moved 'Mesh'");
        }
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
        this->modified = true;
        return index;
    }

    u16 Mesh::vertex_count() const {
        if(this->moved) {
            error("Attempted to use a moved 'Mesh'");
        }
        return this->vertices.size() / this->vertex_size;
    }

    void Mesh::add_element(u16 a, u16 b, u16 c) {
        if(this->moved) {
            error("Attempted to use a moved 'Mesh'");
        }
        this->elements.push_back(a);
        this->elements.push_back(b);
        this->elements.push_back(c);
        this->modified = true;
    }

    u32 Mesh::element_count() const {
        if(this->moved) {
            error("Attempted to use a moved 'Mesh'");
        }
        return this->elements.size() / 3;
    }

    void Mesh::clear() {
        if(this->moved) {
            error("Attempted to use a moved 'Mesh'");
        }
        this->vertices.clear();
        this->elements.clear();
        this->buff_index_count = 0;
        this->modified = true;
    }


    void Mesh::init_buffers() {
        GLuint vbo_id;
        glGenBuffers(1, &vbo_id);
        this->vbo_id = vbo_id;
        GLuint ebo_id;
        glGenBuffers(1, &ebo_id);
        this->ebo_id = ebo_id;
    }

    void Mesh::bind_properties() const {
        u64 vertex_size_b = this->vertex_size * sizeof(f32);
        u64 offset = 0;
        for(u64 attr = 0; attr < this->attrib_sizes.size(); attr += 1) {
            u8 attr_size = this->attrib_sizes[attr];
            glEnableVertexAttribArray(attr);
            glVertexAttribPointer(
                attr, attr_size, 
                GL_FLOAT, GL_FALSE, 
                vertex_size_b, (void*) offset
            );
            offset += attr_size * sizeof(f32);
        }
    }

    void Mesh::unbind_properties() const {
        for(u64 attr = 0; attr < this->attrib_sizes.size(); attr += 1) {
            glDisableVertexAttribArray(attr);
        }
    }

    void Mesh::delete_buffers() const {
        GLuint vbo_id = this->vbo_id;
        glDeleteBuffers(1, &vbo_id);
        GLuint ebo_id = this->ebo_id;
        glDeleteBuffers(1, &ebo_id);
    }


    void Mesh::submit() {
        if(this->moved) {
            error("Attempted to use a moved 'Mesh'");
        }
        if(this->vertices.size() == 0 || this->elements.size() == 0) {
            return;
        }
        glBindBuffer(GL_ARRAY_BUFFER, this->vbo_id);
        glBufferData(
            GL_ARRAY_BUFFER,
            this->vertices.size() * sizeof(f32),
            this->vertices.data(),
            GL_STATIC_DRAW
        );
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo_id);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            this->elements.size() * sizeof(u16),
            this->elements.data(),
            GL_STATIC_DRAW
        );
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        this->buff_index_count = this->elements.size();
        this->modified = false;
    }

    void Mesh::render(const Shader& shader, const Texture& dest) {
        this->internal_render(
            shader, dest.internal_fbo_id(), dest.width(), dest.height()
        );
    }

    void Mesh::internal_render(
        const Shader& shader, u64 dest_fbo_id, 
        i32 dest_width, i32 dest_height
    ) {
        if(this->moved) {
            error("Attempted to use a moved 'Mesh'");
        }
        if(this->modified) { this->submit(); }
        glBindFramebuffer(GL_FRAMEBUFFER, dest_fbo_id);
        glViewport(0, 0, dest_width, dest_height);
        shader.internal_bind();
        glBindBuffer(GL_ARRAY_BUFFER, this->vbo_id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo_id);
        this->bind_properties();
        glDrawElements(
            GL_TRIANGLES, this->buff_index_count, GL_UNSIGNED_SHORT, nullptr
        );
        this->unbind_properties();
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

}