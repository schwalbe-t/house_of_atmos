
#include <engine/rendering.hpp>
#include <engine/logging.hpp>
#include <engine/scene.hpp>
#include <glad/gl.h>
#include <optional>

namespace houseofatmos::engine {

    static GLuint init_fbo() {
        GLuint fbo_id;
        glGenFramebuffers(1, &fbo_id);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);
        return fbo_id;
    }

    static GLuint init_tex(u64 width, u64 height, const void* data) {
        GLuint tex_id;
        glGenTextures(1, &tex_id);
        glBindTexture(GL_TEXTURE_2D, tex_id);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA, 
            width, height, 0, GL_RGBA, 
            GL_UNSIGNED_BYTE, data
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        return tex_id;
    }

    static GLuint init_dbo(u64 width, u64 height) {
        GLuint dbo_id;
        glGenRenderbuffers(1, &dbo_id);
        glBindRenderbuffer(GL_RENDERBUFFER, dbo_id);
        glRenderbufferStorage(
            GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height
        );
        return dbo_id;
    }

    Texture::Texture(u64 width, u64 height, const u8* data) {
        if(width == 0 || height == 0) {
            error("Texture width and height must both be larger than 0"
                " (given was " + std::to_string(width)
                + "x" + std::to_string(height) + ")"
            );
        }
        this->width_px = width;
        this->height_px = height;
        this->fbo_id = init_fbo();
        this->tex_id = init_tex(width, height, (void*) data);
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

    Texture::Texture(u64 width, u64 height) {
        this->moved = true;
        *this = std::move(Texture(width, height, nullptr));
    }

    Texture::Texture(const Image& img) {
        this->moved = true;
        // 'Image' stores the image vertically flipped to how OpenGL expects it
        Image img_flipped = img;
        img_flipped.mirror_vertical();
        *this = std::move(Texture(
            img.width(), img.height(), (const u8*) img_flipped.data()
        ));
    }

    Texture Texture::from_resource(const Texture::LoadArgs& args) {
        auto image = Image::from_resource({ args.path });
        if(args.mirror_vertical) {
            image.mirror_vertical();
        }
        return Texture(image);
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
        };
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


    void Texture::resize_fast(u64 width, u64 height) {
        if(this->width() == width && this->height() == height && !this->moved) {
            return;
        }
        *this = std::move(Texture(width, height));
    }

    void Texture::resize(u64 width, u64 height) {
        if(this->moved) {
            error("Attempted to use a moved 'Texture'");
        }
        if(this->width() == width && this->height() == height) {
            return;
        }
        Texture result = Texture(width, height);
        this->blit(result.as_target(), 0, 0, width, height);
        std::swap(result, *this);
    }


    static std::optional<Shader> blit_shader = std::nullopt;
    static std::optional<Mesh> blit_quad = std::nullopt;

    static void init_blit_resources() {
        blit_shader = Shader(
            "#version 330 \n"
            "layout(location = 0) in vec2 v_pos_uv; \n"
            "out vec2 f_uv; \n"
            "uniform vec2 u_scale; \n"
            "uniform vec2 u_offset; \n"
            "void main() { \n"
            "    gl_Position = vec4(v_pos_uv * u_scale + u_offset, 0.0, 1.0); \n"
            "    f_uv = v_pos_uv; \n"
            "}",
            "#version 130 \n"
            "in vec2 f_uv; \n"
            "uniform sampler2D u_texture; \n"
            "void main() { \n"
            "    gl_FragColor = texture2D(u_texture, f_uv); \n"
            "}"
        );
        Mesh quad = Mesh { { Mesh::F32, 2 } };
        quad.start_vertex();
            quad.put_f32({ 0, 1 });
        u16 tl = quad.complete_vertex();
        quad.start_vertex();
            quad.put_f32({ 1, 1 });
        u16 tr = quad.complete_vertex();
        quad.start_vertex();
            quad.put_f32({ 0, 0 });
        u16 bl = quad.complete_vertex();
        quad.start_vertex();
            quad.put_f32({ 1, 0 });
        u16 br = quad.complete_vertex();
        quad.add_element(tl, bl, br);
        quad.add_element(br, tr, tl);
        quad.submit();
        blit_quad = std::move(quad);
    }

    void Texture::blit(
        RenderTarget dest,
        f64 x, f64 y, f64 w, f64 h
    ) const {
        if(this->moved) {
            error("Attempted to use a moved 'Texture'");
        }
        if(!blit_shader || !blit_quad) { init_blit_resources(); }
        Vec<2> scale = Vec<2>(w, h)
            / Vec<2>(dest.width(), dest.height())
            * 2;
        Vec<2> offset = Vec<2>(x, dest.height() - y - h)
            / Vec<2>(dest.width(), dest.height())
            * 2
            - Vec<2>(1.0, 1.0);
        blit_shader.value().set_uniform("u_scale", scale);
        blit_shader.value().set_uniform("u_offset", offset);
        blit_shader.value().set_uniform("u_texture", *this);
        blit_quad.value().render(
            blit_shader.value(), dest, 1, 
            FaceCulling::Disabled, Rendering::Surfaces, DepthTesting::Disabled
        );
    }

    void Texture::blit(RenderTarget dest, Shader& shader) const {
        if(this->moved) {
            error("Attempted to use a moved 'Texture'");
        }
        if(!blit_quad) { init_blit_resources(); }
        shader.set_uniform("u_texture", *this);
        blit_quad.value().render(
            shader, dest, 1,
            FaceCulling::Disabled, Rendering::Surfaces, DepthTesting::Disabled
        );
    }

}

