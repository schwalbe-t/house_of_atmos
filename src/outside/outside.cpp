
#include "outside.hpp"

#include <glad/gl.h>

namespace houseofatmos::outside {

    void Outside::update(const engine::Window& window) {
        this->time += window.delta_time();
        this->renderer.camera.position += Vec<3>(10, 0, 10) * window.delta_time();
        this->renderer.camera.look_at = this->renderer.camera.position
            + Vec<3>(10, -10, 10);
    }

    void Outside::render(const engine::Window& window) {
        this->renderer.configure(window, *this);
        this->terrain.load_chunks_around(this->renderer.camera.position);
        this->terrain.render_loaded_chunks(
            *this, this->renderer, 
            this->get<engine::Texture>(Outside::terrain_texture)
        );
        window.show_texture(this->renderer.output());
    }

}