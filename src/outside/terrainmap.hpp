
#pragma once

#include "terrain.hpp"
#include <engine/ui.hpp>
#include <engine/rendering.hpp>
#include <engine/localization.hpp>

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

        ui::Element* container = nullptr;

        public:
        Vec<2> view_pos = Vec<2>(0.5, 0.5);
        f64 view_scale = 1.0;

        TerrainMap(const Terrain& terrain): terrain(terrain) {
            this->t_width = terrain.width_in_tiles();
            this->t_height = terrain.height_in_tiles();
            this->rendered_img = engine::Image(this->t_width, this->t_height);
        }

        ui::Element create_container();
        ui::Element* element() const { return this->container; }

        bool toggle_with_key(engine::Key key, const engine::Window& window);
        void update(const engine::Window& window);
        void render_map();
        void render_view();
        
        const engine::Texture& output() const { return this->output_tex; }



        static ui::Element display_item_stack(
            const Item::Stack& stack, const engine::Localization& local,
            f64* text_v_pad_out = nullptr
        );

        static ui::Element display_item_stack_list(
            std::span<const Item::Stack> stacks, 
            const engine::Localization& local,
            f64* text_v_pad_out = nullptr
        );

        static ui::Element display_conversion(
            const Conversion& conv, const engine::Localization& local
        );

        static ui::Element display_building_info(
            Building::Type type, std::span<const Conversion> conversions,
            const engine::Localization& local
        );

    };

}