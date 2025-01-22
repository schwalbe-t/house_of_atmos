
#pragma once

#include "terrain.hpp"
#include <engine/rendering.hpp>

namespace houseofatmos::outside {

    using namespace houseofatmos;


    struct TerrainMap {

        private:
        u64 width, height;
        const Terrain& terrain;
        engine::Image target = engine::Image(0, 0);
        engine::Texture result = engine::Texture(1, 1);

        public:
        TerrainMap(const Terrain& terrain): terrain(terrain) {
            this->width = terrain.width_in_tiles();
            this->height = terrain.height_in_tiles();
            this->target = engine::Image(this->width, this->height);
        }

        void render();
        
        const engine::Image& output_img() const { return this->target; }
        const engine::Texture& output_tex() const { return this->result; }

    };

}