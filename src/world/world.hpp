
#pragma once

#include "../settings.hpp"
#include "terrain.hpp"
#include "complex.hpp"
#include "../player.hpp"
#include "carriage.hpp"
#include "personal_horse.hpp"

namespace houseofatmos::world {

    struct World {

        struct Serialized {
            u64 save_path_len;
            u64 save_path_offset;
            Terrain::Serialized terrain;
            ComplexBank::Serialized complexes;
            Player::Serialized player;
            Balance balance;
            CarriageManager::Serialized carriages;
            PersonalHorse::Serialized personal_horse;
            research::Research::Serialized research;
        };

        static inline u64 units_per_tile = 5;
        static inline u64 tiles_per_chunk = 5;

        static inline f64 autosave_period = 60.0;

        Settings settings;
        std::string save_path;

        Terrain terrain;
        ComplexBank complexes;
        Player player;
        Balance balance;
        CarriageManager carriages;
        PersonalHorse personal_horse;
        research::Research research;

        f64 last_autosave_time;

        private:
        void generate_rivers(StatefulRNG& rng, u32 seed);
        bool settlement_allowed_at(
            u64 center_x, u64 center_z, const std::vector<Vec<3>> settlements
        );
        bool place_building(
            Building::Type type, u64 tile_x, u64 tile_z, 
            std::optional<ComplexId> complex
        );
        void generate_settlement(
            u64 center_x, u64 center_z, StatefulRNG& rng
        );
        std::pair<u64, u64> generate_mansion();
        void generate_map(u32 seed);

        public:
        World(
            Settings&& settings, 
            u64 width = 256, u64 height = 256, u32 seed = random_init()
        );
        World(Settings&& settings, const engine::Arena& serialized);
        
        engine::Arena serialize() const;
        bool write_to_file(bool force_creation = false);

        void trigger_autosave(
            const engine::Window& window, Toasts* toasts = nullptr
        );

    };

}