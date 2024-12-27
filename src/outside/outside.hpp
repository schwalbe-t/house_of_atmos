
#pragma once

#include <engine/window.hpp>
#include <engine/model.hpp>
#include "../renderer.hpp"
#include "terrain.hpp"

namespace houseofatmos::outside {

    struct Outside: engine::Scene {

        Renderer renderer;
        Terrain terrain = Terrain(256, 256, 3);
        f64 time = 0;

        Outside() {
            Terrain::load_resources(*this);
            Renderer::load_shaders(*this);
            Building::load_models(*this);
            //Foliage::load_models(*this);
            this->terrain.generate_elevation();
            this->terrain.generate_foliage();

            this->renderer.camera.position = { 1, 400, 1 };
        }

        void update(const engine::Window& window) override;
        void render(const engine::Window& window) override;

    };

}