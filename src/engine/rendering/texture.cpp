
#include <engine/rendering.hpp>
#include <engine/logging.hpp>
#include <glad/gl.h>

namespace houseofatmos::engine {

    static GLuint init_fbo() {
        GLuint fbo_id;
        glGenFramebuffers(1, &fbo_id);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);
        return fbo_id;
    }

    static GLuint init_tex(u64 width, u64 height) {
        GLuint tex_id;
        glGenTextures(1, &tex_id);
        glBindTexture(GL_TEXTURE_2D, tex_id);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA, 
            width, height, 0, GL_RGBA, 
            GL_UNSIGNED_BYTE, 0
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        return tex_id;
    }

    static GLuint init_dbo(u64 width, u64 height) {
        GLuint dbo_id;
        glGenRenderbuffers(1, &dbo_id);
        glBindRenderbuffer(GL_RENDERBUFFER, dbo_id);
        glRenderbufferStorage(
            GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height
        );
        return dbo_id;
    }

    Texture::Texture(u64 width, u64 height) {
        if(width == 0 || height == 0) {
            error("Attempted to create a texture with size 0");
        }
        this->width_px = width;
        this->height_px = height;
        this->fbo_id = init_fbo();
        this->tex_id = init_tex(width, height);
        this->dbo_id = init_dbo(width, height);
        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, this->dbo_id
        );
        glFramebufferTexture(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, this->tex_id, 0
        );
        bool success = glCheckFramebufferStatus(GL_FRAMEBUFFER)
            == GL_FRAMEBUFFER_COMPLETE;
        if(!success) {
            GLuint fbo_id = this->fbo_id;
            glDeleteFramebuffers(1, &fbo_id);
            GLuint tex_id = this->tex_id;
            glDeleteTextures(1, &tex_id);
            GLuint dbo_id = this->dbo_id;
            glDeleteRenderbuffers(1, &dbo_id);
            error(std::string("Unable to initialize a texture.")
                + " GPU capabilities may be insufficient."
            );
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        this->moved = false;
    }

    Texture::Texture(Texture&& other) noexcept {
        if(other.moved) {
            error("Attempted to move an already moved 'Texture'");
        }
        this->width_px = other.width_px;
        this->height_px = other.height_px;
        this->fbo_id = other.fbo_id;
        this->tex_id = other.tex_id;
        this->dbo_id = other.dbo_id;
        this->moved = false;
        other.moved = true;
    }

    static void delete_resources(GLuint fbo_id, GLuint tex_id, GLuint dbo_id) {
        glDeleteFramebuffers(1, &fbo_id);
        glDeleteTextures(1, &tex_id);
        glDeleteRenderbuffers(1, &dbo_id);
    }

    Texture& Texture::operator=(Texture&& other) noexcept {
        if(this == &other) { return *this; }
        if(other.moved) {
            error("Attempted to move an already moved 'Texture'");
        }
        if(!this->moved) {
            delete_resources(this->fbo_id, this->tex_id, this->dbo_id);
        }
        this->width_px = other.width_px;
        this->height_px = other.height_px;
        this->fbo_id = other.fbo_id;
        this->tex_id = other.tex_id;
        this->dbo_id = other.dbo_id;
        this->moved = false;
        other.moved = true;
        return *this;
    }

    Texture::~Texture() {
        if(this->moved) { return; }
        delete_resources(this->fbo_id, this->tex_id, this->dbo_id);
    }


    u64 Texture::width() const { return this->width_px; }
    u64 Texture::height() const { return this->height_px; }


    void Texture::internal_bind_frame() const {
        glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_id);
        glViewport(0, 0, this->width_px, this->height_px);
    }

    void Texture::internal_unbind_frame() const {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    u64 Texture::internal_fbo_id() const { return this->fbo_id; }
    u64 Texture::internal_tex_id() const { return this->tex_id; }


    void Texture::clear_color(Vec<4> color) const {
        glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_id);
        glClearColor(color.r(), color.g(), color.b(), color.a());
        glClear(GL_COLOR_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Texture::clear_depth(f64 depth) const {
        glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_id);
        glClearDepth(depth);
        glClear(GL_DEPTH_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }


    void Texture::resize_fast(u64 width, u64 height) {
        if(this->width() == width && this->height() == height) {
            return;
        }
        *this = std::move(Texture(width, height));
    }

    void Texture::resize(u64 width, u64 height) {
        if(this->width() == width && this->height() == height) {
            return;
        }
        Texture old = Texture(width, height);
        std::swap(old, *this);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, old.fbo_id);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->fbo_id);
        glBlitFramebuffer(
            0, 0, old.width(),   old.height(),
            0, 0, this->width(), this->height(),
            GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST
        );
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

}

