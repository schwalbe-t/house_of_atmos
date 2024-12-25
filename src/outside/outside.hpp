
#pragma once

#include <engine/window.hpp>
#include <engine/model.hpp>
#include "../renderer.hpp"
#include "buildings.hpp"

namespace houseofatmos::outside {
    
    using namespace houseofatmos;
    using namespace houseofatmos::engine::math;


    struct Terrain {

        u64 width;
        u64 height;
        u64 tile_size = 10;
        u64 chunk_tiles = 8;
        std::vector<u64> elevation;
        std::vector<Building> buildings;

        Terrain(u64 width, u64 height) {
            this->width = width;
            this->height = height;
            this->elevation.resize(width * height);
        }

        u64& elevation_at(u64 x, u64 y) {
            return this->elevation.at(x + this->width * y);
        }
        f64 elevation_at(const Vec<3>& pos);

        void generate_elevation(u32 seed);

    };


    struct Outside: engine::Scene {

        Renderer renderer;
        Terrain terrain = Terrain(128, 128);
        f64 time = 0;

        Outside() {
            Renderer::load_shaders(*this);
            buildings::load_models(*this);
            // this->terrain.generate_elevation(69);

            this->renderer.camera.position = { 7.5, 7.5, 7.5 };
            this->renderer.camera.look_at = { 0, 0, 0 };
        }

        void update(const engine::Window& window) override;
        void render(const engine::Window& window) override;

    };

}