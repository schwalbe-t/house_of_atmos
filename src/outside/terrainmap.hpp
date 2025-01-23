
#pragma once

#include "terrain.hpp"
#include <engine/rendering.hpp>

namespace houseofatmos::outside {

    using namespace houseofatmos;


    struct TerrainMap {

        private:
        u64 t_width, t_height;
        const Terrain& terrain;
        engine::Image rendered_img = engine::Image(0, 0);
        engine::Texture rendered_tex = engine::Texture(1, 1);
        engine::Texture output_tex = engine::Texture(1, 1);

        std::optional<Vec<2>> view_anchor = std::nullopt;

        public:
        Vec<2> view_pos = Vec<2>(0.5, 0.5);
        f64 view_scale = 1.0;
        Vec<2> output_size = Vec<2>(100, 100);

        TerrainMap(const Terrain& terrain): terrain(terrain) {
            this->t_width = terrain.width_in_tiles();
            this->t_height = terrain.height_in_tiles();
            this->rendered_img = engine::Image(this->t_width, this->t_height);
        }

        void adjust_view(const engine::Window& window, Vec<2> display_offset);
        void render_map();
        void render_view();
        
        const engine::Texture& output() const { return this->output_tex; }

    };

}