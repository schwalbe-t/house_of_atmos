
#include <engine/rendering.hpp>
#include <engine/logging.hpp>
#include <glad/gl.h>

namespace houseofatmos::engine {

    static GLint compile_shader(const char* source, GLenum type) {
        GLint id = glCreateShader(type);
        if(id == 0) {
            error("Unable to initialize shader");
        }
        glShaderSource(id, 1, &source, NULL);
        glCompileShader(id);
        GLint compile_status;
        glGetShaderiv(id, GL_COMPILE_STATUS, &compile_status);
        if(compile_status != GL_TRUE) {
            GLint message_len;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &message_len);
            char* message_cstr = new char[message_len];
            glGetShaderInfoLog(id, message_len, &message_len, message_cstr);
            auto message = std::string(message_cstr);
            delete[] message_cstr;
            glDeleteShader(id);
            error("Unable to compile shader:\n" + message);
        }
        return id;
    }

    static GLint link_shaders(GLint vert_id, GLint frag_id) {
        GLint id = glCreateProgram();
        if(id == 0) {
            error("Unable to initialize shader program");
        }
        glAttachShader(id, vert_id);
        glAttachShader(id, frag_id);
        glLinkProgram(id);
        GLint link_status;
        glGetProgramiv(id, GL_LINK_STATUS, &link_status);
        if(link_status != GL_TRUE) {
            GLint message_len;
            glGetProgramiv(id, GL_INFO_LOG_LENGTH, &message_len);
            char* message_cstr = new char[message_len];
            glGetProgramInfoLog(id, message_len, &message_len, message_cstr);
            auto message = std::string(message_cstr);
            delete[] message_cstr;
            glDeleteShader(vert_id);
            glDeleteShader(frag_id);
            glDeleteProgram(id);
            error("Unable to link shaders:\n" + message);
        }
        return id;
    }

    Shader::Shader(const char* vertex_src, const char* fragment_src) {
        this->next_slot = 0;
        this->vert_id = compile_shader(vertex_src, GL_VERTEX_SHADER);
        this->frag_id = compile_shader(fragment_src, GL_FRAGMENT_SHADER);
        this->prog_id = link_shaders(this->vert_id, this->frag_id);
        this->moved = false;
    }

    Shader::Shader(Shader&& other) noexcept {
        if(other.moved) {
            error("Attempted to move an already moved 'Shader'");
        }
        this->uniform_textures = std::move(other.uniform_textures);
        this->texture_uniform_count = std::move(other.texture_uniform_count);
        this->texture_slots = std::move(other.texture_slots);
        this->free_tex_slots = std::move(other.free_tex_slots);
        this->next_slot = other.next_slot;
        this->vert_id = other.vert_id;
        this->frag_id = other.frag_id;
        this->prog_id = other.prog_id;
        this->moved = false;
        other.moved = true;
    }

    Shader& Shader::operator=(Shader&& other) noexcept {
        if(this == &other) { return *this; }
        if(other.moved) {
            error("Attempted to move an already moved 'Shader'");
        }
        if(!this->moved) {
            glDeleteShader(this->vert_id);
            glDeleteShader(this->frag_id);
            glDeleteProgram(this->prog_id);
        }
        this->uniform_textures = std::move(other.uniform_textures);
        this->texture_uniform_count = std::move(other.texture_uniform_count);
        this->texture_slots = std::move(other.texture_slots);
        this->free_tex_slots = std::move(other.free_tex_slots);
        this->next_slot = other.next_slot;
        this->vert_id = other.vert_id;
        this->frag_id = other.frag_id;
        this->prog_id = other.prog_id;
        this->moved = false;
        other.moved = true;
        return *this;
    }

    Shader::~Shader() {
        if(this->moved) { return; }
        glDeleteShader(this->vert_id);
        glDeleteShader(this->frag_id);
        glDeleteProgram(this->prog_id);
    }


    void Shader::internal_bind() const {
        if(this->moved) {
            error("Attempted to use a moved 'Shader'");
        }
        glUseProgram(this->prog_id);
        for(const auto& [tex_id, slot]: this->texture_slots) {
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(GL_TEXTURE_2D, tex_id);
        }
    }


    u64 Shader::max_textures() {
        GLint max_units;
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_units);
        return max_units;
    }


    static GLint binded_uniform_loc(GLuint program, std::string_view name) {
        GLint current_program;
        glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
        if(static_cast<GLuint>(current_program) != program) {
            glUseProgram(program);
        }
        auto name_nt = std::string(name);
        GLint location = glGetUniformLocation(program, name_nt.data());
        if(location == -1) {
            error("The shader does not have any uniform with the name '"
                + std::string(name) + "'"
            );
        }
        return location;
    }

    void Shader::set_uniform(std::string_view name_v, const Texture& texture) {
        auto name = std::string(name_v);
        u64 tex_id = texture.internal_tex_id();
        // if this variable already had a texture, get rid of it
        if(this->uniform_textures.contains(name)) {
            u64 old_tex_id = this->uniform_textures[name];
            u64 old_tex_count = this->texture_uniform_count[old_tex_id];
            old_tex_count -= 1;
            if(old_tex_count > 0) {
                // if there is another uniform still using the texture
                // decrement usage count
                this->texture_uniform_count[old_tex_id] = old_tex_count;
            } else {
                // else get rid of the texture and mark the slot as free
                u64 old_slot = this->texture_slots[old_tex_id];
                this->texture_uniform_count.erase(old_tex_id);
                this->texture_slots.erase(old_tex_id);
                this->free_tex_slots.push_back(old_slot);
            }
        }
        u64 slot;
        if(this->texture_uniform_count.contains(tex_id)) {
            // if there are other uniforms already using the texture
            // use the same slot
            slot = this->texture_slots[tex_id];
        } else if(this->free_tex_slots.size() > 0) {
            // if there are slots that are to be reused reuse them
            slot = this->free_tex_slots.back();
            this->free_tex_slots.pop_back();
        } else {
            // else use the next slot in order
            slot = this->next_slot;
            if(this->next_slot > this->max_textures()) {
                error("Attempted to register more textures at the same time"
                    " than supported ("
                    + std::to_string(this->max_textures())
                    + " for this implementation)"
                );
            }
            this->next_slot += 1;
        }
        // actually assign the texture to the slot
        this->uniform_textures[name] = tex_id;
        this->texture_uniform_count[tex_id] += 1;
        this->texture_slots[tex_id] = slot;
        GLint location = binded_uniform_loc(this->prog_id, name);
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, texture.internal_tex_id());
        glUniform1i(location, slot);
    }


    template<typename T, typename D>
    static std::vector<D> cast_array(std::span<T> v) {
        auto result = std::vector<D>();
        result.reserve(v.size());
        for(size_t v_i = 0; v_i < v.size(); v_i += 1) {
            result.push_back(static_cast<D>(v[v_i]));
        }
        return result; 
    }

    template<int N, typename T, typename D>
    static std::vector<D> flatten_array(std::span<Vec<N, T>> v) {
        auto result = std::vector<D>();
        result.reserve(v.size() * N);
        for(size_t v_i = 0; v_i < v.size(); v_i += 1) {
            for(size_t e_i = 0; e_i < N; e_i += 1) {
                result.push_back(static_cast<D>(v[v_i][e_i]));
            }
        }
        return result;
    }

    void Shader::set_uniform(std::string_view name, f64 v) {
        glUniform1f(binded_uniform_loc(this->prog_id, name), v);
    }
    void Shader::set_uniform(std::string_view name, Vec<2> v) {
        glUniform2f(binded_uniform_loc(this->prog_id, name), v.x(), v.y());
    }
    void Shader::set_uniform(std::string_view name, Vec<3> v) {
        glUniform3f(binded_uniform_loc(this->prog_id, name), v.x(), v.y(), v.z());
    }
    void Shader::set_uniform(std::string_view name, Vec<4> v) {
        glUniform4f(binded_uniform_loc(this->prog_id, name), v.x(), v.y(), v.z(), v.w());
    }
    void Shader::set_uniform(std::string_view name, std::span<f64> value) {
        glUniform1fv(
            binded_uniform_loc(this->prog_id, name), value.size(),
            cast_array<f64, GLfloat>(value).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<Vec<2>> value) {
        glUniform2fv(
            binded_uniform_loc(this->prog_id, name), value.size(),
            flatten_array<2, f64, GLfloat>(value).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<Vec<3>> value) {
        glUniform3fv(
            binded_uniform_loc(this->prog_id, name), value.size(),
            flatten_array<3, f64, GLfloat>(value).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<Vec<4>> value) {
        glUniform4fv(
            binded_uniform_loc(this->prog_id, name), value.size(),
            flatten_array<4, f64, GLfloat>(value).data()
        );
    }


    void Shader::set_uniform(std::string_view name, i64 v) {
        glUniform1i(binded_uniform_loc(this->prog_id, name), v);
    }
    void Shader::set_uniform(std::string_view name, IVec<2> v) {
        glUniform2i(binded_uniform_loc(this->prog_id, name), v.x(), v.y());
    }
    void Shader::set_uniform(std::string_view name, IVec<3> v) {
        glUniform3i(binded_uniform_loc(this->prog_id, name), v.x(), v.y(), v.z());
    }
    void Shader::set_uniform(std::string_view name, IVec<4> v) {
        glUniform4i(binded_uniform_loc(this->prog_id, name), v.x(), v.y(), v.z(), v.w());
    }
    void Shader::set_uniform(std::string_view name, std::span<i64> value) {
        glUniform1iv(
            binded_uniform_loc(this->prog_id, name), value.size(),
            cast_array<i64, GLint>(value).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<IVec<2>> value) {
        glUniform2iv(
            binded_uniform_loc(this->prog_id, name), value.size(),
            flatten_array<2, i64, GLint>(value).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<IVec<3>> value) {
        glUniform3iv(
            binded_uniform_loc(this->prog_id, name), value.size(),
            flatten_array<3, i64, GLint>(value).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<IVec<4>> value) {
        glUniform4iv(
            binded_uniform_loc(this->prog_id, name), value.size(),
            flatten_array<4, i64, GLint>(value).data()
        );
    }


    void Shader::set_uniform(std::string_view name, u64 v) {
        glUniform1i(binded_uniform_loc(this->prog_id, name), v);
    }
    void Shader::set_uniform(std::string_view name, UVec<2> v) {
        glUniform2i(binded_uniform_loc(this->prog_id, name), v.x(), v.y());
    }
    void Shader::set_uniform(std::string_view name, UVec<3> v) {
        glUniform3i(binded_uniform_loc(this->prog_id, name), v.x(), v.y(), v.z());
    }
    void Shader::set_uniform(std::string_view name, UVec<4> v) {
        glUniform4i(binded_uniform_loc(this->prog_id, name), v.x(), v.y(), v.z(), v.w());
    }
    void Shader::set_uniform(std::string_view name, std::span<u64> value) {
        glUniform1uiv(
            binded_uniform_loc(this->prog_id, name), value.size(),
            cast_array<u64, GLuint>(value).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<UVec<2>> value) {
        glUniform2uiv(
            binded_uniform_loc(this->prog_id, name), value.size(),
            flatten_array<2, u64, GLuint>(value).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<UVec<3>> value) {
        glUniform3uiv(
            binded_uniform_loc(this->prog_id, name), value.size(),
            flatten_array<3, u64, GLuint>(value).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<UVec<4>> value) {
        glUniform4uiv(
            binded_uniform_loc(this->prog_id, name), value.size(),
            flatten_array<4, u64, GLuint>(value).data()
        );
    }


    template<int R, int C>
    static std::vector<GLfloat> flatten_matrices(
        const Mat<R, C>* values, size_t count
    ) {
        auto result = std::vector<GLfloat>();
        result.reserve(R * C);
        for(size_t m_i = 0; m_i < count; m_i += 1) {
            for(size_t r_i = 0; r_i < R; r_i += 1) {
                for(size_t c_i = 0; c_i < C; c_i += 1) {
                    f64 elem = values[m_i].element(r_i, c_i);
                    result.push_back(static_cast<GLfloat>(elem));
                }
            }
        }
        return result;
    }

    void Shader::set_uniform(std::string_view name, const Mat<2>& value) {
        glUniformMatrix2fv(
            binded_uniform_loc(this->prog_id, name), 1, GL_FALSE,
            flatten_matrices<2, 2>(&value, 1).data()
        );
    }
    void Shader::set_uniform(std::string_view name, const Mat<3>& value) {
        glUniformMatrix3fv(
            binded_uniform_loc(this->prog_id, name), 1, GL_FALSE,
            flatten_matrices<3, 3>(&value, 1).data()
        );
    }
    void Shader::set_uniform(std::string_view name, const Mat<4>& value) {
        glUniformMatrix4fv(
            binded_uniform_loc(this->prog_id, name), 1, GL_FALSE,
            flatten_matrices<4, 4>(&value, 1).data()
        );
    }
    void Shader::set_uniform(std::string_view name, const Mat<2, 3>& value) {
        glUniformMatrix3x2fv(
            binded_uniform_loc(this->prog_id, name), 1, GL_FALSE,
            flatten_matrices<2, 3>(&value, 1).data()
        );
    }
    void Shader::set_uniform(std::string_view name, const Mat<3, 2>& value) {
        glUniformMatrix2x3fv(
            binded_uniform_loc(this->prog_id, name), 1, GL_FALSE,
            flatten_matrices<3, 2>(&value, 1).data()
        );
    }
    void Shader::set_uniform(std::string_view name, const Mat<2, 4>& value) {
        glUniformMatrix4x2fv(
            binded_uniform_loc(this->prog_id, name), 1, GL_FALSE,
            flatten_matrices<2, 4>(&value, 1).data()
        );
    }
    void Shader::set_uniform(std::string_view name, const Mat<4, 2>& value) {
        glUniformMatrix2x4fv(
            binded_uniform_loc(this->prog_id, name), 1, GL_FALSE,
            flatten_matrices<4, 2>(&value, 1).data()
        );
    }
    void Shader::set_uniform(std::string_view name, const Mat<3, 4>& value) {
        glUniformMatrix4x3fv(
            binded_uniform_loc(this->prog_id, name), 1, GL_FALSE,
            flatten_matrices<3, 4>(&value, 1).data()
        );
    }
    void Shader::set_uniform(std::string_view name, const Mat<4, 3>& value) {
        glUniformMatrix3x4fv(
            binded_uniform_loc(this->prog_id, name), 1, GL_FALSE,
            flatten_matrices<4, 3>(&value, 1).data()
        );
    }

    void Shader::set_uniform(std::string_view name, std::span<Mat<2>> value) {
        glUniformMatrix2fv(
            binded_uniform_loc(this->prog_id, name), value.size(), GL_FALSE,
            flatten_matrices<2, 2>(value.data(), value.size()).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<Mat<3>> value) {
        glUniformMatrix3fv(
            binded_uniform_loc(this->prog_id, name), value.size(), GL_FALSE,
            flatten_matrices<3, 3>(value.data(), value.size()).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<Mat<4>> value) {
        glUniformMatrix4fv(
            binded_uniform_loc(this->prog_id, name), value.size(), GL_FALSE,
            flatten_matrices<4, 4>(value.data(), value.size()).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<Mat<2, 3>> value) {
        glUniformMatrix3x2fv(
            binded_uniform_loc(this->prog_id, name), value.size(), GL_FALSE,
            flatten_matrices<2, 3>(value.data(), value.size()).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<Mat<3, 2>> value) {
        glUniformMatrix2x3fv(
            binded_uniform_loc(this->prog_id, name), value.size(), GL_FALSE,
            flatten_matrices<3, 2>(value.data(), value.size()).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<Mat<2, 4>> value) {
        glUniformMatrix4x2fv(
            binded_uniform_loc(this->prog_id, name), value.size(), GL_FALSE,
            flatten_matrices<2, 4>(value.data(), value.size()).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<Mat<4, 2>> value) {
        glUniformMatrix2x4fv(
            binded_uniform_loc(this->prog_id, name), value.size(), GL_FALSE,
            flatten_matrices<4, 2>(value.data(), value.size()).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<Mat<3, 4>> value) {
        glUniformMatrix4x3fv(
            binded_uniform_loc(this->prog_id, name), value.size(), GL_FALSE,
            flatten_matrices<3, 4>(value.data(), value.size()).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<Mat<4, 3>> value) {
        glUniformMatrix3x4fv(
            binded_uniform_loc(this->prog_id, name), value.size(), GL_FALSE,
            flatten_matrices<4, 3>(value.data(), value.size()).data()
        );
    }

}