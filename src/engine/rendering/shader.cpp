
#include <engine/rendering.hpp>
#include <engine/logging.hpp>
#include <engine/scene.hpp>
#include <glad/gl.h>
#include <filesystem>

namespace fs = std::filesystem;

namespace houseofatmos::engine {

    static std::string expand_shader_includes(
        const std::string& source, const std::string& source_path
    ) {
        const std::string include_start = "#include ";
        std::string result = source;
        for(;;) {
            // can be one of
            // - '#include <SOME_FILE>'
            // - '#include "SOME_FILE"'
            size_t macro_start_p = result.find(include_start);
            if(macro_start_p == std::string::npos) { break; }
            size_t open_p = macro_start_p + include_start.size();
            if(open_p + 2 >= result.size()) { break; }
            char open_c = result[open_p];
            char close_c = open_c;
            if(open_c == '<') { close_c = '>'; }
            else if(open_c != '\"') { break; }
            size_t close_p = result.find(close_c, open_p + 1);
            if(close_p == std::string::npos) { break; }
            std::string path = result.substr(open_p + 1, close_p - open_p - 1);
            // '#include <SOME_FILE>' => relative to working directory
            // '#include "SOME_FILE"' => relative to 'source_path'
            if(open_c == '\"') {
                fs::path source_dir = fs::path(source_path).parent_path();
                fs::path include_path = source_dir / path;
                path = include_path.string();
            }
            if(!fs::exists(path)) {
                info("While expanding include from '" + source_path + "':");
            }
            std::string contents = GenericResource::read_string(path);
            std::string expanded = expand_shader_includes(contents, path);
            result = result.substr(0, macro_start_p)
                + expanded
                + result.substr(close_p + 1);
        }
        return result;
    }

    static GLint compile_shader(
        std::string raw_source, const std::string& source_path, GLenum type
    ) {
        GLint id = glCreateShader(type);
        if(id == 0) {
            error("Unable to initialize shader");
        }
        std::string expanded_source = expand_shader_includes(
            raw_source, source_path
        );
        const char* expanded_source_cstr = expanded_source.c_str();
        glShaderSource(id, 1, &expanded_source_cstr, NULL);
        glCompileShader(id);
        GLint compile_status;
        glGetShaderiv(id, GL_COMPILE_STATUS, &compile_status);
        if(compile_status != GL_TRUE) {
            GLint message_len;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &message_len);
            if(message_len == 0) {
                glDeleteShader(id);
                std::string type_str = type == GL_VERTEX_SHADER
                    ? "vertex shader" : "fragment shader";
                info("While compiling "+type_str+" at '"+source_path+"':");
                error("Unable to compile shader (no log available)");
            }
            auto message = std::string(message_len, '\0');
            glGetShaderInfoLog(id, message_len, &message_len, message.data());
            message.resize(message_len);
            glDeleteShader(id);
            std::string type_str = type == GL_VERTEX_SHADER
                ? "vertex shader" : "fragment shader";
            info("While compiling "+type_str+" at '"+source_path+"':");
            error("Unable to compile shader:\n" + message);
        }
        return id;
    }

    static GLint link_shaders(
        GLint vert_id, GLint frag_id,
        const std::string& vertex_path, const std::string& fragment_path
    ) {
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
            if(message_len == 0) {
                glDeleteShader(vert_id);
                glDeleteShader(frag_id);
                glDeleteProgram(id);
                info("While linking program from '" + vertex_path
                    + "' and '" + fragment_path + "':"
                );
                error("Unable to link shaders (no log available)");
            }
            auto message = std::string(message_len, '\0');
            glGetProgramInfoLog(id, message_len, &message_len, message.data());
            message.resize(message_len);
            glDeleteShader(vert_id);
            glDeleteShader(frag_id);
            glDeleteProgram(id);
            info("While linking program from '" + vertex_path
                + "' and '" + fragment_path + "':"
            );
            error("Unable to link shaders:\n" + message);
        }
        return id;
    }

    Shader::Shader(
        const std::string& vertex_src, const std::string& fragment_src,
        const std::string& vertex_file, const std::string& fragment_file
    ) {
        this->next_slot = 0;
        this->vert_id = compile_shader(
            vertex_src.data(), vertex_file, GL_VERTEX_SHADER
        );
        this->frag_id = compile_shader(
            fragment_src.data(), fragment_file, GL_FRAGMENT_SHADER
        );
        this->prog_id = link_shaders(
            this->vert_id, this->frag_id, vertex_file, fragment_file
        );
        this->moved = false;
    }

    Shader Shader::from_resource(const Shader::LoadArgs& args) {
        std::string vertex_src = GenericResource::read_string(args.vertex_path);
        std::string fragment_src = GenericResource::read_string(args.fragment_path);
        return Shader(
            vertex_src, fragment_src, args.vertex_path, args.fragment_path
        );
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
            for(const auto& [tex_id, slot_info]: this->texture_slots) {
                bool is_array = slot_info.second;
                if(!is_array) { continue; }
                GLuint tex_gid = tex_id;
                glDeleteTextures(1, &tex_gid);
            }
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
        for(const auto& [tex_id, slot_info]: this->texture_slots) {
            bool is_array = slot_info.second;
            if(!is_array) { continue; }
            GLuint tex_gid = tex_id;
            glDeleteTextures(1, &tex_gid);
        }
        glDeleteShader(this->vert_id);
        glDeleteShader(this->frag_id);
        glDeleteProgram(this->prog_id);
    }


    void Shader::internal_bind() const {
        if(this->moved) {
            error("Attempted to use a moved 'Shader'");
        }
        glUseProgram(this->prog_id);
        for(const auto& [tex_id, slot_info]: this->texture_slots) {
            const auto& [slot, is_array] = slot_info;
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(is_array? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D, tex_id);
        }
    }

    void Shader::internal_unbind() const {
        if(this->moved) {
            error("Attempted to use a moved 'Shader'");
        }
        for(const auto& [tex_id, slot_info]: this->texture_slots) {
            const auto& [slot, is_array] = slot_info;
            glActiveTexture(GL_TEXTURE0 + slot);
            glBindTexture(is_array? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D, 0);
        }
        glUseProgram(0);
    }


    size_t Shader::max_textures() {
        GLint max_units;
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_units);
        return (size_t) max_units;
    }


    static GLint binded_uniform_loc(
        bool moved, GLuint program, std::string_view name
    ) {
        if(moved) {
            error("Attempted to use a moved 'Shader'");
        }
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

    u64 Shader::allocate_texture_slot(
        std::string name, u64 tex_id, bool is_array
    ) {
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
                const auto& [old_slot, old_was_array] 
                    = this->texture_slots[old_tex_id];
                this->texture_uniform_count.erase(old_tex_id);
                this->texture_slots.erase(old_tex_id);
                this->free_tex_slots.push_back(old_slot);
                if(old_was_array) {
                    GLuint old_tex_gid = old_tex_id;
                    glDeleteTextures(1, &old_tex_gid);
                }
            }
        }
        // determine the slot
        u64 slot;
        if(this->texture_uniform_count.contains(tex_id)) {
            // if there are other uniforms already using the texture
            // use the same slot
            slot = this->texture_slots[tex_id].first;
        } else if(this->free_tex_slots.size() > 0) {
            // if there are slots that are to be reused reuse them
            slot = this->free_tex_slots.back();
            this->free_tex_slots.pop_back();
        } else {
            // else use the next slot in order
            slot = this->next_slot;
            if(this->next_slot > Shader::max_textures()) {
                error("Attempted to register more textures at the same time"
                    " than supported ("
                    + std::to_string(Shader::max_textures())
                    + " for this implementation)"
                );
            }
            this->next_slot += 1;
        }
        // use the slot
        this->texture_uniform_count[tex_id] += 1; 
        this->uniform_textures[name] = tex_id;
        this->texture_slots[tex_id] = { slot, is_array };
        return slot;
    }

    void Shader::set_uniform(std::string_view name_v, const Texture& texture) {
        if(this->moved) {
            error("Attempted to use a moved 'Shader'");
        }
        auto name = std::string(name_v);
        u64 tex_id = texture.internal_tex_id();
        u64 slot = this->allocate_texture_slot(name, tex_id, false /* not tex array */);
        GLint location = binded_uniform_loc(this->moved, this->prog_id, name);
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, tex_id);
        glUniform1i(location, slot);
    }

    void Shader::set_uniform(
        std::string_view name_v, std::span<const Texture*> textures
    ) {
        if(this->moved) {
            error("Attempted to use a moved 'Shader'");
        }
        if(textures.size() == 0) { return; }
        const Texture& first_texture = *textures[0];
        auto name = std::string(name_v);
        GLuint array_id;
        glGenTextures(1, &array_id);
        glBindTexture(GL_TEXTURE_2D_ARRAY, array_id);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_REPEAT);
        glTexImage3D(
            GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, 
            first_texture.width(), first_texture.height(), textures.size(), 
            0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr
        );
        for(size_t i = 0; i < textures.size(); i += 1) {
            const Texture& texture = *textures[i];
            bool size_valid = texture.width() == first_texture.width()
                && texture.height() == first_texture.height();
            if(!size_valid) {
                engine::error("Texture sizes don't match!"
                    " (while setting uniform '" + name + "')"
                );
            }
            glBindFramebuffer(GL_FRAMEBUFFER, texture.internal_fbo_id());
            glCopyTexSubImage3D(
                GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, 
                0, 0, texture.width(), texture.height()
            );
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        u64 slot = this->allocate_texture_slot(name, array_id, true /* is tex array */);
        GLint location = binded_uniform_loc(this->moved, this->prog_id, name);
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D_ARRAY, array_id);
        glUniform1i(location, slot);
    }

    void Shader::set_uniform(
        std::string_view name_v, std::span<const Texture> textures
    ) {
        std::vector<const Texture*> texture_ptrs;
        texture_ptrs.reserve(textures.size());
        for(const Texture& texture: textures) {
            texture_ptrs.push_back(&texture);
        }
        this->set_uniform(name_v, texture_ptrs);
    }


    template<typename T, typename D>
    static std::vector<D> cast_array(std::span<const T> v) {
        auto result = std::vector<D>();
        result.reserve(v.size());
        for(size_t v_i = 0; v_i < v.size(); v_i += 1) {
            result.push_back(static_cast<D>(v[v_i]));
        }
        return result; 
    }

    template<int N, typename T, typename D>
    static std::vector<D> flatten_array(std::span<const Vec<N, T>> v) {
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
        glUniform1f(
            binded_uniform_loc(this->moved, this->prog_id, name), v
        );
    }
    void Shader::set_uniform(std::string_view name, Vec<2> v) {
        glUniform2f(
            binded_uniform_loc(this->moved, this->prog_id, name), v.x(), v.y()
        );
    }
    void Shader::set_uniform(std::string_view name, Vec<3> v) {
        glUniform3f(
            binded_uniform_loc(this->moved, this->prog_id, name), 
            v.x(), v.y(), v.z()
        );
    }
    void Shader::set_uniform(std::string_view name, Vec<4> v) {
        glUniform4f(
            binded_uniform_loc(this->moved, this->prog_id, name), 
            v.x(), v.y(), v.z(), v.w()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<const f64> value) {
        glUniform1fv(
            binded_uniform_loc(this->moved, this->prog_id, name), value.size(),
            cast_array<f64, GLfloat>(value).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<const Vec<2>> value) {
        glUniform2fv(
            binded_uniform_loc(this->moved, this->prog_id, name), value.size(),
            flatten_array<2, f64, GLfloat>(value).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<const Vec<3>> value) {
        glUniform3fv(
            binded_uniform_loc(this->moved, this->prog_id, name), value.size(),
            flatten_array<3, f64, GLfloat>(value).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<const Vec<4>> value) {
        glUniform4fv(
            binded_uniform_loc(this->moved, this->prog_id, name), value.size(),
            flatten_array<4, f64, GLfloat>(value).data()
        );
    }


    void Shader::set_uniform(std::string_view name, i64 v) {
        glUniform1i(binded_uniform_loc(this->moved, this->prog_id, name), v);
    }
    void Shader::set_uniform(std::string_view name, IVec<2> v) {
        glUniform2i(
            binded_uniform_loc(this->moved, this->prog_id, name), v.x(), v.y()
        );
    }
    void Shader::set_uniform(std::string_view name, IVec<3> v) {
        glUniform3i(
            binded_uniform_loc(this->moved, this->prog_id, name), 
            v.x(), v.y(), v.z()
        );
    }
    void Shader::set_uniform(std::string_view name, IVec<4> v) {
        glUniform4i(
            binded_uniform_loc(this->moved, this->prog_id, name), 
            v.x(), v.y(), v.z(), v.w()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<const i64> value) {
        glUniform1iv(
            binded_uniform_loc(this->moved, this->prog_id, name), value.size(),
            cast_array<i64, GLint>(value).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<const IVec<2>> value) {
        glUniform2iv(
            binded_uniform_loc(this->moved, this->prog_id, name), value.size(),
            flatten_array<2, i64, GLint>(value).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<const IVec<3>> value) {
        glUniform3iv(
            binded_uniform_loc(this->moved, this->prog_id, name), value.size(),
            flatten_array<3, i64, GLint>(value).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<const IVec<4>> value) {
        glUniform4iv(
            binded_uniform_loc(this->moved, this->prog_id, name), value.size(),
            flatten_array<4, i64, GLint>(value).data()
        );
    }


    void Shader::set_uniform(std::string_view name, u64 v) {
        glUniform1i(binded_uniform_loc(this->moved, this->prog_id, name), v);
    }
    void Shader::set_uniform(std::string_view name, UVec<2> v) {
        glUniform2i(
            binded_uniform_loc(this->moved, this->prog_id, name), v.x(), v.y()
        );
    }
    void Shader::set_uniform(std::string_view name, UVec<3> v) {
        glUniform3i(
            binded_uniform_loc(this->moved, this->prog_id, name), 
            v.x(), v.y(), v.z()
        );
    }
    void Shader::set_uniform(std::string_view name, UVec<4> v) {
        glUniform4i(
            binded_uniform_loc(this->moved, this->prog_id, name),
            v.x(), v.y(), v.z(), v.w()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<const u64> value) {
        glUniform1uiv(
            binded_uniform_loc(this->moved, this->prog_id, name), value.size(),
            cast_array<u64, GLuint>(value).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<const UVec<2>> value) {
        glUniform2uiv(
            binded_uniform_loc(this->moved, this->prog_id, name), value.size(),
            flatten_array<2, u64, GLuint>(value).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<const UVec<3>> value) {
        glUniform3uiv(
            binded_uniform_loc(this->moved, this->prog_id, name), value.size(),
            flatten_array<3, u64, GLuint>(value).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<const UVec<4>> value) {
        glUniform4uiv(
            binded_uniform_loc(this->moved, this->prog_id, name), value.size(),
            flatten_array<4, u64, GLuint>(value).data()
        );
    }


    template<int R, int C>
    static std::vector<GLfloat> flatten_matrices(
        const Mat<R, C>* values, size_t count
    ) {
        auto result = std::vector<GLfloat>();
        result.reserve(R * C * count);
        for(size_t m_i = 0; m_i < count; m_i += 1) {
            const Mat<R, C>& matrix = values[m_i];
            std::vector<GLfloat> flat = matrix.template as_column_major<GLfloat>();
            result.insert(result.end(), flat.begin(), flat.end());
        }
        return result;
    }

    void Shader::set_uniform(std::string_view name, const Mat<2>& value) {
        glUniformMatrix2fv(
            binded_uniform_loc(this->moved, this->prog_id, name), 1, GL_FALSE,
            flatten_matrices<2, 2>(&value, 1).data()
        );
    }
    void Shader::set_uniform(std::string_view name, const Mat<3>& value) {
        glUniformMatrix3fv(
            binded_uniform_loc(this->moved, this->prog_id, name), 1, GL_FALSE,
            flatten_matrices<3, 3>(&value, 1).data()
        );
    }
    void Shader::set_uniform(std::string_view name, const Mat<4>& value) {
        glUniformMatrix4fv(
            binded_uniform_loc(this->moved, this->prog_id, name), 1, GL_FALSE,
            flatten_matrices<4, 4>(&value, 1).data()
        );
    }
    void Shader::set_uniform(std::string_view name, const Mat<2, 3>& value) {
        glUniformMatrix3x2fv(
            binded_uniform_loc(this->moved, this->prog_id, name), 1, GL_FALSE,
            flatten_matrices<2, 3>(&value, 1).data()
        );
    }
    void Shader::set_uniform(std::string_view name, const Mat<3, 2>& value) {
        glUniformMatrix2x3fv(
            binded_uniform_loc(this->moved, this->prog_id, name), 1, GL_FALSE,
            flatten_matrices<3, 2>(&value, 1).data()
        );
    }
    void Shader::set_uniform(std::string_view name, const Mat<2, 4>& value) {
        glUniformMatrix4x2fv(
            binded_uniform_loc(this->moved, this->prog_id, name), 1, GL_FALSE,
            flatten_matrices<2, 4>(&value, 1).data()
        );
    }
    void Shader::set_uniform(std::string_view name, const Mat<4, 2>& value) {
        glUniformMatrix2x4fv(
            binded_uniform_loc(this->moved, this->prog_id, name), 1, GL_FALSE,
            flatten_matrices<4, 2>(&value, 1).data()
        );
    }
    void Shader::set_uniform(std::string_view name, const Mat<3, 4>& value) {
        glUniformMatrix4x3fv(
            binded_uniform_loc(this->moved, this->prog_id, name), 1, GL_FALSE,
            flatten_matrices<3, 4>(&value, 1).data()
        );
    }
    void Shader::set_uniform(std::string_view name, const Mat<4, 3>& value) {
        glUniformMatrix3x4fv(
            binded_uniform_loc(this->moved, this->prog_id, name), 1, GL_FALSE,
            flatten_matrices<4, 3>(&value, 1).data()
        );
    }

    void Shader::set_uniform(std::string_view name, std::span<const Mat<2>> value) {
        glUniformMatrix2fv(
            binded_uniform_loc(this->moved, this->prog_id, name), 
            value.size(), GL_FALSE,
            flatten_matrices<2, 2>(value.data(), value.size()).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<const Mat<3>> value) {
        glUniformMatrix3fv(
            binded_uniform_loc(this->moved, this->prog_id, name), 
            value.size(), GL_FALSE,
            flatten_matrices<3, 3>(value.data(), value.size()).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<const Mat<4>> value) {
        glUniformMatrix4fv(
            binded_uniform_loc(this->moved, this->prog_id, name), 
            value.size(), GL_FALSE,
            flatten_matrices<4, 4>(value.data(), value.size()).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<const Mat<2, 3>> value) {
        glUniformMatrix3x2fv(
            binded_uniform_loc(this->moved, this->prog_id, name), 
            value.size(), GL_FALSE,
            flatten_matrices<2, 3>(value.data(), value.size()).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<const Mat<3, 2>> value) {
        glUniformMatrix2x3fv(
            binded_uniform_loc(this->moved, this->prog_id, name), 
            value.size(), GL_FALSE,
            flatten_matrices<3, 2>(value.data(), value.size()).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<const Mat<2, 4>> value) {
        glUniformMatrix4x2fv(
            binded_uniform_loc(this->moved, this->prog_id, name),
            value.size(), GL_FALSE,
            flatten_matrices<2, 4>(value.data(), value.size()).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<const Mat<4, 2>> value) {
        glUniformMatrix2x4fv(
            binded_uniform_loc(this->moved, this->prog_id, name),
            value.size(), GL_FALSE,
            flatten_matrices<4, 2>(value.data(), value.size()).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<const Mat<3, 4>> value) {
        glUniformMatrix4x3fv(
            binded_uniform_loc(this->moved, this->prog_id, name),
            value.size(), GL_FALSE,
            flatten_matrices<3, 4>(value.data(), value.size()).data()
        );
    }
    void Shader::set_uniform(std::string_view name, std::span<const Mat<4, 3>> value) {
        glUniformMatrix3x4fv(
            binded_uniform_loc(this->moved, this->prog_id, name),
            value.size(), GL_FALSE,
            flatten_matrices<4, 3>(value.data(), value.size()).data()
        );
    }

}