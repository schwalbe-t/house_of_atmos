
#pragma once

#include <engine/arena.hpp>
#include <engine/window.hpp>
#include <engine/model.hpp>
#include "../renderer.hpp"
#include "../player.hpp"
#include "terrain.hpp"
#include "actionmode.hpp"

namespace houseofatmos::outside {

    struct Outside: engine::Scene {

        struct Serialized {
            Terrain::Serialized terrain;
            Player::Serialized player;
            Balance balance;
        };


        static inline const char* const save_location = "savegame.bin"; 

        static inline const i64 draw_distance = 1;
        static inline const u64 units_per_tile = 5;
        static inline const u64 tiles_per_chunk = 8;


        Renderer renderer;
        Terrain terrain = Terrain(
            256, 256, draw_distance, units_per_tile, tiles_per_chunk
        );
        Player player;
        std::unique_ptr<ActionMode> action_mode;
        Balance balance;

        Outside();
        Outside(const engine::Arena& buffer);

        void update(engine::Window& window) override;
        void render(engine::Window& window) override;

        engine::Arena serialize() const;

    };

}