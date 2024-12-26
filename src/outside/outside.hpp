
#pragma once

#include <engine/window.hpp>
#include <engine/model.hpp>
#include "../renderer.hpp"
#include "terrain.hpp"

namespace houseofatmos::outside {

    struct Outside: engine::Scene {

        static inline engine::Texture::LoadArgs terrain_texture = {
            "res/terrain.png"
        };

        Renderer renderer;
        Terrain terrain = Terrain(128, 128);
        f64 time = 0;

        Outside() {
            this->load(engine::Texture::Loader(Outside::terrain_texture));
            Renderer::load_shaders(*this);
            Building::load_models(*this);
            //Foliage::load_models(*this);
            this->terrain.generate_elevation(69);
            this->terrain.generate_foliage(69);

            this->renderer.camera.position = { 1, 10, 1 };
        }

        void update(const engine::Window& window) override;
        void render(const engine::Window& window) override;

    };

}