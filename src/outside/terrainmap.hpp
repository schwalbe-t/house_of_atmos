
#pragma once

#include <engine/ui.hpp>
#include <engine/rendering.hpp>
#include <engine/localization.hpp>
#include "terrain.hpp"
#include "carriage.hpp"

namespace houseofatmos::outside {

    using namespace houseofatmos;


    struct TerrainMap {

        private:
        u64 t_width, t_height;
        const ui::Localization::LoadArgs& local_ref;
        const ui::Localization* local = nullptr;
        const Terrain& terrain;
        const ComplexBank& complexes;
        const Player& player;
        CarriageManager& carriages;
        ui::Manager& ui;
        engine::Image rendered_img = engine::Image(0, 0);
        engine::Texture rendered_tex = engine::Texture(1, 1);
        engine::Texture output_tex = engine::Texture(1, 1);
        Vec<2> view_size_px;
        Vec<2> view_pos_px;

        std::optional<Vec<2>> view_anchor = std::nullopt;

        ui::Element* container = nullptr;
        ui::Element* selected_info_right = nullptr;
        ui::Element* selected_info_bottom = nullptr;

        std::optional<ComplexId> selected_complex = std::nullopt;

        bool hovering_marker();
        void update_view(const engine::Window& window);
        void update_click(const engine::Window& window);

        void render_view();
        void add_marker(
            Vec<2> pos, const ui::Background* icon, 
            std::function<void ()>&& handler, bool is_phantom = false
        );
        void create_markers();

        public:
        Vec<2> view_pos = Vec<2>(0.5, 0.5);
        f64 view_scale = 1.0;

        TerrainMap(
            const ui::Localization::LoadArgs& local,
            const Terrain& terrain, const ComplexBank& complexes,
            const Player& player, CarriageManager& carriages, ui::Manager& ui
        ): local_ref(local), terrain(terrain), complexes(complexes), 
            player(player), carriages(carriages), ui(ui) {
            this->t_width = terrain.width_in_tiles();
            this->t_height = terrain.height_in_tiles();
            this->rendered_img = engine::Image(this->t_width, this->t_height);
        }

        void create_container();
        ui::Element* element() const { return this->container; }

        bool toggle_with_key(engine::Key key, const engine::Window& window);
        void update(const engine::Window& window, engine::Scene& scene);
        void render_map();
        void render();



        static ui::Element display_item_stack(
            Item::Type type, const std::string& count, 
            const engine::Localization& local, f64* text_v_pad_out = nullptr
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

        static ui::Element display_complex_info(
            const Complex& complex, const engine::Localization& local
        );

    };

}