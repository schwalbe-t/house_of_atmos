
#include <engine/rendering.hpp>
#include <glad/gles2.h>

namespace houseofatmos::engine {

    void RenderTarget::clear_color(Vec<4> color) const {
        glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_id);
        glClearColor(color.r(), color.g(), color.b(), color.a());
        glClear(GL_COLOR_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void RenderTarget::clear_depth(f64 depth) const {
        glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_id);
        glClearDepthf((f32) depth);
        glClear(GL_DEPTH_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

}