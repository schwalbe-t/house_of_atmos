
#include <engine/rendering.hpp>
#include <glad/gl.h>

namespace houseofatmos::engine {

    GLuint create_texture_array(
        u64 width, u64 height, size_t layers, 
        std::span<const Texture*> textures
    ) {
        GLuint array_id;
        glGenTextures(1, &array_id);
        glBindTexture(GL_TEXTURE_2D_ARRAY, array_id);
        glTexImage3D(
            GL_TEXTURE_2D_ARRAY, 0, GL_RGBA,
            width, height, layers, 
            0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr
        );
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_REPEAT);
        for(size_t layer_i = 0; layer_i < textures.size(); layer_i += 1) {
            const Texture& texture = *textures[layer_i];
            bool size_valid = texture.width() == width
                && texture.height() == height;
            if(!size_valid) {
                engine::error("Texture sizes don't match!"
                    " (while creating a 'TextureArray')"
                );
            }
            glBindFramebuffer(GL_FRAMEBUFFER, texture.as_target().fbo_id);
            glCopyTexSubImage3D(
                GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer_i, 
                0, 0, texture.width(), texture.height()
            );
        }
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        return array_id;
    }

    std::pair<GLuint, GLuint> create_layer_fbo(
        GLuint array_id, u64 width, u64 height, size_t layer_i
    ) {
        GLuint fbo_id;
        glGenFramebuffers(1, &fbo_id);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);
        glFramebufferTextureLayer(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, array_id, 0, layer_i
        );
        GLuint dbo_id;
        glGenRenderbuffers(1, &dbo_id);
        glBindRenderbuffer(GL_RENDERBUFFER, dbo_id);
        glRenderbufferStorage(
            GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height
        );
        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, dbo_id
        );
        bool success = glCheckFramebufferStatus(GL_FRAMEBUFFER)
            == GL_FRAMEBUFFER_COMPLETE;
        if(!success) {
            glDeleteRenderbuffers(1, &dbo_id);
            glDeleteFramebuffers(1, &fbo_id);
            glDeleteTextures(1, &array_id);
            error(std::string("Unable to initialize a texture array.")
                + " GPU capabilities may be insufficient."
            );
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return { fbo_id, dbo_id };
    }

    void TextureArray::init(
        u64 width, u64 height, size_t layers, 
        std::span<const Texture*> textures
    ) {
        this->width_px = width;
        this->height_px = height;
        this->tex_id = 0;
        this->fbo_ids.reserve(layers);
        this->dbo_ids.reserve(layers);
        this->moved = false;
        if(layers == 0) { return; }
        GLuint tex_id = create_texture_array(width, height, layers, textures);
        this->tex_id = (u64) tex_id;
        for(size_t layer_i = 0; layer_i < layers; layer_i += 1) {
            const auto& [fbo_id, dbo_id]
                = create_layer_fbo(tex_id, width, height, layer_i);
            this->fbo_ids.push_back((u64) fbo_id);
            this->dbo_ids.push_back((u64) dbo_id);
        }
    }

    void TextureArray::deallocate() const {
        if(this->tex_id == 0) { return; }
        for(size_t layer_i = 0; layer_i < this->fbo_ids.size(); layer_i += 1) {
            GLuint dbo_id = this->dbo_ids[layer_i];
            glDeleteRenderbuffers(1, &dbo_id);
            GLuint fbo_id = this->fbo_ids[layer_i];
            glDeleteFramebuffers(1, &fbo_id);
        }
        GLuint tex_id = this->tex_id;
        glDeleteTextures(1, &tex_id);
    }

    TextureArray::TextureArray(u64 width, u64 height, size_t layers) {
        this->init(width, height, layers, std::span<const Texture*>());
    }

    TextureArray::TextureArray(std::span<const Texture*> textures) {
        if(textures.size() == 0) {
            this->init(0, 0, 0, textures);
            return;
        }
        this->init(
            textures[0]->width(), textures[0]->height(), 
            textures.size(), textures
        );
    }

    TextureArray::TextureArray(std::span<const Texture> textures) {
        if(textures.size() == 0) {
            this->init(0, 0, 0, std::span<const Texture*>());
            return;
        }
        std::vector<const Texture*> texture_ptrs;
        texture_ptrs.reserve(textures.size());
        for(const Texture& texture: textures) {
            texture_ptrs.push_back(&texture);
        }
        this->init(
            textures[0].width(), textures[1].height(),
            textures.size(), texture_ptrs
        );
    }

    TextureArray::TextureArray(TextureArray&& other) noexcept {
        if(other.moved) {
            error("Attempted to move an already moved 'TextureArray'");
        }
        this->width_px = other.width_px;
        this->height_px = other.width_px;
        this->tex_id = other.tex_id;
        this->fbo_ids = std::move(other.fbo_ids);
        this->dbo_ids = std::move(other.dbo_ids);
        this->moved = false;
        other.moved = true;
    }

    TextureArray& TextureArray::operator=(TextureArray&& other) noexcept {
        if(this == &other) { return *this; }
        if(other.moved) {
            error("Attempted to move an already moved 'TextureArray'");
        }
        if(!this->moved) {
            this->deallocate();
        }
        this->width_px = other.width_px;
        this->height_px = other.width_px;
        this->tex_id = other.tex_id;
        this->fbo_ids = std::move(other.fbo_ids);
        this->dbo_ids = std::move(other.dbo_ids);
        this->moved = false;
        other.moved = true;
        return *this;
    }

    TextureArray::~TextureArray() {
        if(this->moved) { return; }
        this->deallocate();
    }

}