
#pragma once

#include <engine/localization.hpp>
#include <engine/arena.hpp>
#include <engine/window.hpp>
#include <engine/model.hpp>
#include <engine/ui.hpp>
#include "../ui_const.hpp"
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
            u64 save_path_len;
            u64 save_path_offset;
            Terrain::Serialized terrain;
            ComplexBank::Serialized complexes;
            Player::Serialized player;
            Balance balance;
            CarriageManager::Serialized carriages;
        };


        static inline const u64 units_per_tile = 5;
        static inline const u64 tiles_per_chunk = 8;
        static inline const i64 draw_distance_ch = 1;
        static inline const i64 draw_distance_un    
            = draw_distance_ch * tiles_per_chunk * units_per_tile;

        static inline const f64 min_camera_dist = 15.0;
        static inline const f64 max_camera_dist = 50.0;

        std::string save_path;
        engine::Localization::LoadArgs local;

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
        ui::Element* coins_elem = nullptr;
        ui::Manager ui = ui::Manager(ui_const::unit_size_fract);
        TerrainMap terrain_map = TerrainMap(
            Outside::local, this->terrain, this->complexes, this->player, 
            this->carriages, this->ui
        );
        Toasts toasts = Toasts(this->local);

        Outside(
            engine::Localization::LoadArgs local
        );
        Outside(
            engine::Localization::LoadArgs local, 
            const engine::Arena& buffer
        );
        void load_resources();

        void update(engine::Window& window) override;
        void render(engine::Window& window) override;

        engine::Arena serialize() const;

    };

}