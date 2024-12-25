
#include "outside.hpp"

#include <glad/gl.h>

namespace houseofatmos::outside {

    void Outside::update(const engine::Window& window) {
        this->time += window.delta_time();
    }

    void Outside::render(const engine::Window& window) {
        this->renderer.configure(window, *this);
        this->renderer.render(
            this->get<engine::Model>(buildings::farmland.model),
            Mat<4>::rotate_y(this->time)
        );
        window.show_texture(this->renderer.output());
    }

}