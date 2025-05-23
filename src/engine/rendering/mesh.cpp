
#include <engine/rendering.hpp>
#include <engine/logging.hpp>
#include <glad/gles2.h>
#include <numeric>

namespace houseofatmos::engine {

    size_t Mesh::Attrib::type_size_bytes() const {
        switch(this->type) {
            case Mesh::F32: return sizeof(f32);
            case Mesh::I8: return sizeof(i8);
            case Mesh::I16: return sizeof(i16);
            case Mesh::I32: return sizeof(i32);
            case Mesh::U8: return sizeof(u8);
            case Mesh::U16: return sizeof(u16);
            case Mesh::U32: return sizeof(u32);
        }
        error("Unhandled mesh attribute type in 'Mesh::Attrib::type_size_bytes'");
    }

    size_t Mesh::Attrib::size_bytes() const {
        return this->type_size_bytes() * this->count;
    }

    int Mesh::Attrib::gl_type_constant() const {
        switch(this->type) {
            case Mesh::F32: return GL_FLOAT;
            case Mesh::I8: return GL_BYTE;
            case Mesh::I16: return GL_SHORT;
            case Mesh::I32: return GL_INT;
            case Mesh::U8: return GL_UNSIGNED_BYTE;
            case Mesh::U16: return GL_UNSIGNED_SHORT;
            case Mesh::U32: return GL_UNSIGNED_INT;
        }
        error("Unhandled mesh attribute type in 'Mesh::Attrib::gl_type_constant'");
    }

    std::string Mesh::Attrib::display_type() const {
        switch(this->type) {
            case Mesh::F32: return "f32";
            case Mesh::I8: return "i8";
            case Mesh::I16: return "i16"; 
            case Mesh::I32: return "i32";
            case Mesh::U8: return "u8";
            case Mesh::U16: return "u16";
            case Mesh::U32: return "u32";
        }
        error("Unhandled mesh attribute type in 'Mesh::Attrib::display_type'");
    }

    std::string Mesh::Attrib::display() const {
        return std::to_string(this->count) + "x'" + this->display_type() + "'";
    }



    size_t Mesh::max_attributes() {
        GLint max_attribs;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_attribs);
        return (size_t) max_attribs;
    }

    void Mesh::destruct(const Handles& handles) {
        GLuint vbo_id = handles.vbo_id;
        glDeleteBuffers(1, &vbo_id);
        GLuint ebo_id = handles.ebo_id;
        glDeleteBuffers(1, &ebo_id);
    }

    static size_t compute_vertex_size(const std::vector<Mesh::Attrib>& attribs) {
        size_t sum = 0;
        for(const Mesh::Attrib& attrib: attribs) {
            sum += attrib.size_bytes();
        }
        return sum;
    }

    Mesh::Mesh(std::span<const Attrib> attrib_sizes) {
        if(attrib_sizes.size() > Mesh::max_attributes()) {
            error("Attempted to create a mesh with more attributes"
                " than supported ("
                + std::to_string(Mesh::max_attributes())
                + " for this implementation)"
            );
        }
        this->attributes.assign(attrib_sizes.begin(), attrib_sizes.end());
        this->vertex_size = compute_vertex_size(this->attributes);
        this->vertices = 0;
        this->current_attrib = 0;
        this->init_buffers();
        this->modified = false;
    }

    Mesh::Mesh(std::initializer_list<Attrib> attrib_sizes) {
        *this = Mesh(std::span(attrib_sizes));
    }


    void Mesh::start_vertex() {
        if(this->current_attrib != 0) {
            error("Attempted to build multiple mesh vertices at the same time");
        }
    }

    static void assert_current_attrib(
        const std::vector<Mesh::Attrib>& attributes, size_t expected_attrib_i, 
        Mesh::Attrib got
    ) {
        if(expected_attrib_i >= attributes.size()) {
            error("Mesh expected no more data for the current vertex, but got "
                + got.display()
            );
        }
        const Mesh::Attrib& expected = attributes[expected_attrib_i];
        if(expected.type != got.type || expected.count != got.count) {
            error("Mesh expected "
                + expected.display()
                + " for the current vertex, but got "
                + got.display()
            );
        }
    }

    template<typename T>
    static void append_attrib_data(
        std::vector<u8>& vertex_data, std::span<const T> values
    ) {
        size_t existing_length = vertex_data.size();
        size_t copied_length = values.size() * sizeof(T);
        vertex_data.resize(existing_length + copied_length);
        std::memcpy(
            (void*) (vertex_data.data() + existing_length), 
            (void*) values.data(), copied_length
        );
    }

    void Mesh::put_f32(std::span<const f32> values) {
        assert_current_attrib(
            this->attributes, this->current_attrib, { Mesh::F32, values.size() }
        );
        append_attrib_data(this->vertex_data, values);
        this->unsafe_next_attr();
    }
    void Mesh::put_f32(std::initializer_list<f32> values) { this->put_f32(std::span(values)); }

    void Mesh::put_i8(std::span<const i8> values) {
        assert_current_attrib(
            this->attributes, this->current_attrib, { Mesh::I8, values.size() }
        );
        append_attrib_data(this->vertex_data, values);
        this->unsafe_next_attr();
    }
    void Mesh::put_i8(std::initializer_list<i8> values) { this->put_i8(std::span(values)); }

    void Mesh::put_i16(std::span<const i16> values) {
        assert_current_attrib(
            this->attributes, this->current_attrib, { Mesh::I16, values.size() }
        );
        append_attrib_data(this->vertex_data, values);
        this->unsafe_next_attr();
    }
    void Mesh::put_i16(std::initializer_list<i16> values) { this->put_i16(std::span(values)); }

    void Mesh::put_i32(std::span<const i32> values) {
        assert_current_attrib(
            this->attributes, this->current_attrib, { Mesh::I32, values.size() }
        );
        append_attrib_data(this->vertex_data, values);
        this->unsafe_next_attr();
    }
    void Mesh::put_i32(std::initializer_list<i32> values) { this->put_i32(std::span(values)); }

    void Mesh::put_u8(std::span<const u8> values) {
        assert_current_attrib(
            this->attributes, this->current_attrib, { Mesh::U8, values.size() }
        );
        append_attrib_data(this->vertex_data, values);
        this->unsafe_next_attr();
    }
    void Mesh::put_u8(std::initializer_list<u8> values) { this->put_u8(std::span(values)); }

    void Mesh::put_u16(std::span<const u16> values) {
        assert_current_attrib(
            this->attributes, this->current_attrib, { Mesh::U16, values.size() }
        );
        append_attrib_data(this->vertex_data, values);
        this->unsafe_next_attr();
    }
    void Mesh::put_u16(std::initializer_list<u16> values) { this->put_u16(std::span(values)); }

    void Mesh::put_u32(std::span<const u32> values) {
        assert_current_attrib(
            this->attributes, this->current_attrib, { Mesh::U32, values.size() }
        );
        append_attrib_data(this->vertex_data, values);
        this->unsafe_next_attr();
    }
    void Mesh::put_u32(std::initializer_list<u32> values) { this->put_u32(std::span(values)); }

    void Mesh::unsafe_put_raw(std::span<const u8> data) {
        append_attrib_data(this->vertex_data, data);
    }

    void Mesh::unsafe_next_attr() {
        this->current_attrib += 1;
    }

    u16 Mesh::complete_vertex() {
        if(this->current_attrib < this->attributes.size()) {
            const Attrib& attribute = this->attributes[this->current_attrib];
            error("Mesh expected "
                + attribute.display()
                + " for the current vertex, but got the end of the vertex"
            );
        }
        this->current_attrib = 0;
        u16 vertex = this->vertices;
        this->vertices += 1;
        this->modified = true;
        return vertex;
    }


    u16 Mesh::vertex_count() const {
        return this->vertices;
    }

    void Mesh::add_element(u16 a, u16 b, u16 c) {
        if(a >= this->vertices || b >= this->vertices || c >= this->vertices) {
            error("we fucked up big time");
        }
        this->elements.push_back(a);
        this->elements.push_back(b);
        this->elements.push_back(c);
        this->modified = true;
    }

    u32 Mesh::element_count() const {
        return this->elements.size() / 3;
    }

    void Mesh::clear() {
        this->vertex_data.clear();
        this->vertices = 0;
        this->current_attrib = 0;
        this->elements.clear();
        this->modified = true;
    }


    void Mesh::init_buffers() {
        GLuint vbo_id;
        glGenBuffers(1, &vbo_id);
        GLuint ebo_id;
        glGenBuffers(1, &ebo_id);
        this->handles = util::Handle<Handles, &Mesh::destruct>(
            Handles(vbo_id, ebo_id)
        );
    }

    void Mesh::bind_properties() const {
        u64 offset = 0;
        for(u64 attr_i = 0; attr_i < this->attributes.size(); attr_i += 1) {
            const Attrib& attribute = this->attributes[attr_i];
            glEnableVertexAttribArray(attr_i);
            if(attribute.type == AttribType::F32) {
                glVertexAttribPointer(
                    attr_i, attribute.count, 
                    GL_FLOAT, GL_FALSE, 
                    this->vertex_size, (void*) offset
                );
            } else {
                glVertexAttribIPointer(
                    attr_i, attribute.count, 
                    attribute.gl_type_constant(), 
                    this->vertex_size, (void*) offset
                );                
            }
            offset += attribute.size_bytes();
        }
    }

    void Mesh::unbind_properties() const {
        for(u64 attr = 0; attr < this->attributes.size(); attr += 1) {
            glDisableVertexAttribArray(attr);
        }
    }


    void Mesh::submit() {
        if(!this->modified) { return; }
        if(this->vertices > 0 && this->elements.size() > 0) {
            glBindBuffer(GL_ARRAY_BUFFER, this->handles->vbo_id);
            glBufferData(
                GL_ARRAY_BUFFER,
                this->vertex_data.size(),
                this->vertex_data.data(),
                GL_STATIC_DRAW
            );
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        if(this->vertices > 0 && this->elements.size() > 0) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->handles->ebo_id);
            glBufferData(
                GL_ELEMENT_ARRAY_BUFFER,
                this->elements.size() * sizeof(u16),
                this->elements.data(),
                GL_STATIC_DRAW
            );
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }
        this->modified = false;
    }

    void Mesh::render(
        const Shader& shader, RenderTarget dest,
        size_t count, FaceCulling face_culling, 
        DepthTesting depth_testing
    ) {
        if(count == 0) { return; }
        if(this->modified) { this->submit(); }
        if(face_culling == FaceCulling::Enabled) {
            glEnable(GL_CULL_FACE);
        }
        if(depth_testing == DepthTesting::Enabled) { 
            glEnable(GL_DEPTH_TEST); 
            glCullFace(GL_BACK);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, dest.fbo_id);
        glViewport(0, 0, dest.width(), dest.height());
        shader.internal_bind();
        glBindBuffer(GL_ARRAY_BUFFER, this->handles->vbo_id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->handles->ebo_id);
        this->bind_properties();
        if(count == 1) {
            glDrawElements(
                GL_TRIANGLES, this->elements.size(), GL_UNSIGNED_SHORT, nullptr
            );
        } else {
            glDrawElementsInstanced(
                GL_TRIANGLES, this->elements.size(), GL_UNSIGNED_SHORT, nullptr, 
                count
            );
        }
        this->unbind_properties();
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        shader.internal_unbind();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if(depth_testing == DepthTesting::Enabled) { 
            glDisable(GL_DEPTH_TEST);
        }
        if(face_culling == FaceCulling::Enabled) {
            glDisable(GL_CULL_FACE);
        }
    }

}