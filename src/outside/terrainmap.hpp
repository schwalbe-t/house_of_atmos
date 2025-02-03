
#pragma once

#include <engine/ui.hpp>
#include <engine/rendering.hpp>
#include <engine/localization.hpp>
#include "terrain.hpp"
#include "carriage.hpp"
#include "personal_horse.hpp"

namespace houseofatmos::outside {

    using namespace houseofatmos;


    struct TerrainMap {

        private:
        u64 t_width, t_height;
        const engine::Localization::LoadArgs& local_ref;
        const engine::Localization* local = nullptr;
        const Terrain& terrain;
        const ComplexBank& complexes;
        const Player& player;
        CarriageManager& carriages;
        const PersonalHorse& personal_horse;
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
        std::optional<u64> selected_carriage = std::nullopt;
        bool adding_stop = false;

        void update_view(const engine::Window& window);
        void update_click(const engine::Window& window);

        void render_view();
        void create_marker_info();
        void add_marker(Vec<2> pos, ui::Element&& element);
        void add_icon_marker(
            Vec<2> pos, const ui::Background* icon, 
            std::function<void ()>&& handler, bool is_phantom = false
        );
        void create_markers();

        public:
        Vec<2> view_pos = Vec<2>(0.5, 0.5);
        f64 view_scale = 1.0;

        TerrainMap(
            const engine::Localization::LoadArgs& local,
            const Terrain& terrain, const ComplexBank& complexes,
            const Player& player, CarriageManager& carriages, 
            const PersonalHorse& personal_horse, ui::Manager& ui
        ): local_ref(local), terrain(terrain), complexes(complexes), 
            player(player), carriages(carriages), personal_horse(personal_horse), 
            ui(ui) {
            this->t_width = terrain.width_in_tiles();
            this->t_height = terrain.height_in_tiles();
            this->rendered_img = engine::Image(this->t_width, this->t_height);
        }

        void create_container();
        ui::Element* element() const { return this->container; }

        void hide();
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

        static ui::Element create_selection_container(std::string title);

        static ui::Element create_selection_item(
            const ui::Background* icon, std::string text, bool selected,
            std::function<void ()>&& handler
        );

        static ui::Element display_item_selector(
            std::span<const Item::Type> items, 
            std::function<void (Item::Type)>&& handler,
            const engine::Localization& local
        );

        ui::Element display_carriage_target(Carriage* carriage, size_t target_i);

        ui::Element display_carriage_info(Carriage& carriage);

    };

}