
#include <engine/rendering.hpp>
#include <engine/logging.hpp>
#include <engine/scene.hpp>
#include <glad/gl.h>
#include <stb/stb_image.h>
#include <optional>

namespace houseofatmos::engine {

    static GLuint init_fbo() {
        GLuint fbo_id;
        glGenFramebuffers(1, &fbo_id);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);
        return fbo_id;
    }

    static GLuint init_tex(i64 width, i64 height, const void* data) {
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
        return tex_id;
    }

    static GLuint init_dbo(i64 width, i64 height) {
        GLuint dbo_id;
        glGenRenderbuffers(1, &dbo_id);
        glBindRenderbuffer(GL_RENDERBUFFER, dbo_id);
        glRenderbufferStorage(
            GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height
        );
        return dbo_id;
    }

    Texture::Texture(i64 width, i64 height, const u8* data) {
        if(width <= 0 || height <= 0) {
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

    Texture::Texture(i64 width, i64 height) {
        this->moved = true;
        *this = std::move(Texture(width, height, nullptr));
    }

    Texture Texture::from_resource(const Texture::LoadArgs& args) {
        std::vector<char> bytes = GenericResource::read_bytes(args.path);
        int width, height, file_channels;
        stbi_uc* data = stbi_load_from_memory(
            (stbi_uc*) bytes.data(), bytes.size(), 
            &width, &height, nullptr, STBI_rgb_alpha
        );
        if(data == nullptr) {
            error("The file '" + args.path + "' contains invalid image data!");
        }
        auto instance = Texture(width, height, (u8*) data);
        stbi_image_free((void*) data);
        return instance;
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


    i64 Texture::width() const { return this->width_px; }
    i64 Texture::height() const { return this->height_px; }


    u64 Texture::internal_fbo_id() const {
        if(this->moved) {
            error("Attempted to use a moved 'Texture'");
        }
        return this->fbo_id;
    }
    u64 Texture::internal_tex_id() const {
        if(this->moved) {
            error("Attempted to use a moved 'Texture'");
        }
        return this->tex_id;
    }


    void Texture::clear_color(Vec<4> color) const {
        if(this->moved) {
            error("Attempted to use a moved 'Texture'");
        }
        glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_id);
        glClearColor(color.r(), color.g(), color.b(), color.a());
        glClear(GL_COLOR_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Texture::clear_depth(f64 depth) const {
        if(this->moved) {
            error("Attempted to use a moved 'Texture'");
        }
        glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_id);
        glClearDepth(depth);
        glClear(GL_DEPTH_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }


    void Texture::resize_fast(i64 width, i64 height) {
        if(this->width() == width && this->height() == height && !this->moved) {
            return;
        }
        *this = std::move(Texture(width, height));
    }

    void Texture::resize(i64 width, i64 height) {
        if(this->moved) {
            error("Attempted to use a moved 'Texture'");
        }
        if(this->width() == width && this->height() == height) {
            return;
        }
        Texture result = Texture(width, height);
        this->blit(result, 0, 0, width, height);
        std::swap(result, *this);
    }


    void Texture::blit(const Texture& dest, f64 x, f64 y, f64 w, f64 h) const {
        if(this->moved) {
            error("Attempted to use a moved 'Texture'");
        }
        this->internal_blit(
            dest.internal_fbo_id(), dest.width(), dest.height(),
            x, y, w, h
        );
    }

    static std::optional<Shader> blit_shader = std::nullopt;
    static std::optional<Mesh> blit_quad = std::nullopt;

    static void init_blit_resources() {
        blit_shader = Shader(
            "#version 130 \n"
            "in vec2 v_pos_uv; \n"
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

    void Texture::internal_blit(
        u64 dest_fbo_id, u32 dest_width, u32 dest_height,
        f64 x, f64 y, f64 w, f64 h
    ) const {
        if(this->moved) {
            error("Attempted to use a moved 'Texture'");
        }
        if(!blit_shader || !blit_quad) { init_blit_resources(); }
        Vec<2> scale = Vec<2>(w, h)
            / Vec<2>(dest_width, dest_height)
            * 2;
        Vec<2> offset = Vec<2>(x, y)
            / Vec<2>(dest_width, dest_height)
            * 2
            - Vec<2>(1.0, 1.0);
        blit_shader.value().set_uniform("u_scale", scale);
        blit_shader.value().set_uniform("u_offset", offset);
        blit_shader.value().set_uniform("u_texture", *this);
        blit_quad.value().internal_render(
            blit_shader.value(), dest_fbo_id, dest_width, dest_height, false
        );
    }

}

