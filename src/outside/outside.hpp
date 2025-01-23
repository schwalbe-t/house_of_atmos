
#pragma once

#include <engine/localization.hpp>
#include <engine/arena.hpp>
#include <engine/window.hpp>
#include <engine/model.hpp>
#include <engine/ui.hpp>
#include "../ui_background.hpp"
#include "../ui_font.hpp"
#include "../ui_icon.hpp"
#include "../renderer.hpp"
#include "../player.hpp"
#include "terrain.hpp"
#include "actionmode.hpp"
#include "carriage.hpp"
#include "terrainmap.hpp"

namespace houseofatmos::outside {

    namespace ui = houseofatmos::engine::ui;


    struct Outside: engine::Scene {

        struct Serialized {
            Terrain::Serialized terrain;
            ComplexBank::Serialized complexes;
            Player::Serialized player;
            Balance balance;
            CarriageManager::Serialized carriages;
        };


        static inline const engine::Localization::LoadArgs local = {
            "res/localization.json", "de"
        };

        static inline const char* const save_location = "savegame.bin"; 

        static inline const u64 units_per_tile = 5;
        static inline const u64 tiles_per_chunk = 8;
        static inline const i64 draw_distance_ch = 1;
        static inline const i64 draw_distance_un 
            = draw_distance_ch * tiles_per_chunk * units_per_tile;
        static inline const f64 ui_unit_size = 1 / 250.0;

        static inline const f64 min_camera_dist = 15.0;
        static inline const f64 max_camera_dist = 50.0;

        Renderer renderer;
        Terrain terrain = Terrain(
            256, 256, draw_distance_ch, units_per_tile, tiles_per_chunk
        );
        ComplexBank complexes;
        Player player;
        Balance balance;
        CarriageManager carriages;

        f64 camera_distance = min_camera_dist;
        std::unique_ptr<ActionMode> action_mode;
        TerrainMap terrain_map = TerrainMap(this->terrain);
        ui::Element* map;
        ui::Manager ui = std::move(ui::Manager(ui_unit_size)
            .with_element(std::move(ui::Element()
                .with_pos(0.05, 0.05, ui::position::window_fract)
                .with_size(0, 0, ui::size::units_with_children)
                .with_background(&ui_background::scroll_horizontal)
                .with_list_dir(ui::Direction::Horizontal)
                .with_child(ui::Element()
                    .with_size(16, 16, ui::size::units)
                    .with_background(&ui_icon::terrain)
                    .with_padding(0)
                    .with_background(
                        &ui_background::border,
                        &ui_background::border_select
                    )
                    .with_padding(2)
                    .as_movable()
                )
                .with_child(ui::Element()
                    .with_size(16, 16, ui::size::units)
                    .with_background(&ui_icon::construction)
                    .with_padding(0)
                    .with_background(
                        &ui_background::border,
                        &ui_background::border_select
                    )
                    .with_padding(2)
                    .as_movable()
                )
                .with_child(ui::Element()
                    .with_size(16, 16, ui::size::units)
                    .with_background(&ui_icon::demolition)
                    .with_padding(0)
                    .with_background(
                        &ui_background::border,
                        &ui_background::border_select
                    )
                    .with_padding(2)
                    .as_movable()
                )
                .with_child(ui::Element()
                    .with_size(16, 16, ui::size::units)
                    .with_background(&ui_icon::pathing)
                    .with_padding(0)
                    .with_background(
                        &ui_background::border,
                        &ui_background::border_select
                    )
                    .with_padding(2)
                    .as_movable()
                )
            ))
            .with_element(ui::Element()
                .with_pos(0.5, 0.5, ui::position::window_fract)
                .with_size(0.9, 0.9, ui::size::window_fract)
                .with_background(&ui_background::scroll_horizontal)
                .with_handle(&map)
                .as_hidden(true)
                .as_movable()
            )
        );

        Outside();
        Outside(const engine::Arena& buffer);

        void update(engine::Window& window) override;
        void render(engine::Window& window) override;

        engine::Arena serialize() const;

    };

}