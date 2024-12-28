
#include "outside.hpp"

namespace houseofatmos::outside {

    void Outside::update(const engine::Window& window) {
        this->time += window.delta_time();
        Vec<3> vel = { 0, 0, 0 };
        if(window.is_down(engine::Key::A)) { vel.x() -= 1; }
        if(window.is_down(engine::Key::D)) { vel.x() += 1; }
        if(window.is_down(engine::Key::W)) { vel.z() -= 1; }
        if(window.is_down(engine::Key::S)) { vel.z() += 1; }
        this->target += vel.normalized() * 30 * window.delta_time();
        this->target.y() = this->terrain.elevation_at(this->target);
        this->renderer.camera.position = this->target + Vec<3>(0, 25, 50);
        this->renderer.camera.look_at = this->target;
    }

    void Outside::render(const engine::Window& window) {
        this->renderer.configure(window, *this);
        this->terrain.load_chunks_around(this->target);
        this->terrain.render_loaded_chunks(*this, this->renderer);
        window.show_texture(this->renderer.output());
        //engine::debug("FPS: " + std::to_string(1.0 / window.delta_time()));
    }

}