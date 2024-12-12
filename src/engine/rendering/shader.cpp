
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
        this->vert_id = compile_shader(vertex_src, GL_VERTEX_SHADER);
        this->frag_id = compile_shader(fragment_src, GL_FRAGMENT_SHADER);
        this->prog_id = link_shaders(this->vert_id, this->frag_id);
        this->moved = false;
    }

    Shader::Shader(Shader&& other) noexcept {
        if(other.moved) {
            error("Attempted to move an already moved 'Shader'");
        }
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
    }

    void Shader::internal_unbind() const { glUseProgram(0); }

}