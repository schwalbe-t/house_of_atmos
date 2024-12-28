
#include "outside.hpp"

namespace houseofatmos::outside {

    Outside::Outside() {
        Renderer::load_shaders(*this);
        Terrain::load_resources(*this);
        Building::load_models(*this);
        Foliage::load_models(*this);
        Player::load_model(*this);
        this->terrain.generate_elevation();
        this->terrain.generate_foliage();
        this->player.position = { 250, 0, 250 };
    }

    void Outside::update(const engine::Window& window) {
        this->player.update(window);
        this->player.position.y() = std::max(
            this->terrain.elevation_at(this->player.position),
            -1.7
        );
        this->player.in_water = this->player.position.y() <= -1.5;
        this->renderer.camera.look_at = this->player.position;
        this->renderer.camera.position = this->player.position
            + Vec<3>(0, 12, 12);
    }

    void Outside::render(const engine::Window& window) {
        this->renderer.configure(window, *this);
        this->terrain.load_chunks_around(this->player.position);
        this->terrain.render_loaded_chunks(*this, this->renderer, window);
        this->player.render(*this, this->renderer);
        window.show_texture(this->renderer.output());
        engine::debug("FPS: " + std::to_string(1.0 / window.delta_time()));
    }

}