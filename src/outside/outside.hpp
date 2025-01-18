
#pragma once

#include <engine/arena.hpp>
#include <engine/window.hpp>
#include <engine/model.hpp>
#include <engine/ui.hpp>
#include "../ui_backgrounds.hpp"
#include "../renderer.hpp"
#include "../player.hpp"
#include "terrain.hpp"
#include "actionmode.hpp"
#include "carriage.hpp"

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


        static inline const char* const save_location = "savegame.bin"; 

        static inline const u64 units_per_tile = 5;
        static inline const u64 tiles_per_chunk = 8;
        static inline const i64 draw_distance_ch = 1;
        static inline const i64 draw_distance_un 
            = draw_distance_ch * tiles_per_chunk * units_per_tile;
        static inline const f64 ui_unit_size = 1 / 250.0;

        Renderer renderer;
        Terrain terrain = Terrain(
            256, 256, draw_distance_ch, units_per_tile, tiles_per_chunk
        );
        ComplexBank complexes;
        Player player;
        Balance balance;
        CarriageManager carriages;

        std::unique_ptr<ActionMode> action_mode;
        Zoom::Level zoom = Zoom::Near;
        ui::Manager ui = std::move(
            ui::Manager(ui_unit_size)
                .with_element(std::move(
                    ui::Element()
                        .with_pos(0.05, 0.5, ui::position::window_fract)
                        .with_size(64, 128, ui::size::units)
                        .with_background(&ui_background::scroll_vertical)
                ))
                .with_element(std::move(
                    ui::Element()
                        .with_pos(0.05, 0.05, ui::position::window_fract)
                        .with_size(100, 16 + 2 + 2, ui::size::units)
                        .with_background(&ui_background::scroll_horizontal)
                ))
        );

        Outside();
        Outside(const engine::Arena& buffer);

        void update(engine::Window& window) override;
        void render(engine::Window& window) override;

        engine::Arena serialize() const;

    };

}