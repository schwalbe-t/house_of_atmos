
#pragma once

#include "../settings.hpp"
#include "terrain.hpp"
#include "complex.hpp"
#include "../player.hpp"
#include "carriage.hpp"
#include "train.hpp"
#include "boat.hpp"
#include "personal_horse.hpp"
#include "population.hpp"

namespace houseofatmos::world {

    struct World {

        // For any breaking change to the serialized structure of the world,
        // increment this number by 1
        // (any time an update makes it so old files can't be loaded anymore)
        static inline u32 current_format_version = 6;

        struct Serialized {
            u32 format_version;
            Terrain::Serialized terrain;
            ComplexBank::Serialized complexes;
            Player::Serialized player;
            Balance balance;
            CarriageManager::Serialized carriages;
            TrainManager::Serialized trains;
            BoatManager::Serialized boats;
            PersonalHorse::Serialized personal_horse;
            research::Research::Serialized research;
            PopulationManager::Serialized populations;
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
        TrainManager trains;
        BoatManager boats;
        PersonalHorse personal_horse;
        research::Research research;
        PopulationManager populations;

        bool saving_allowed = true;
        f64 last_autosave_time;

        private:
        void generate_rivers(StatefulRNG& rng, u32 seed);
        bool settlement_allowed_at(
            u64 center_x, u64 center_z,
            const std::vector<std::pair<u64, u64>> settlements
        );
        std::pair<u64, u64> generate_mansion();

        void trigger_autosave(const engine::Window& window, Toasts& toasts);

        public:
        World(
            Settings&& settings, 
            u64 width = 256, u64 height = 256
        );
        World(Settings&& settings, const engine::Arena& serialized);

        World(World&& other) noexcept = delete;
        World& operator=(World&& other) noexcept = delete;

        void generate_map(u32 seed);
        
        engine::Arena serialize() const;
        bool write_to_file(bool force_creation = false);

        void update(
            engine::Scene& scene, const engine::Window& window, Toasts& toasts,
            ParticleManager* particles = nullptr, 
            Interactables* interactables = nullptr
        );


        static inline const size_t max_game_name_len = 10;

        static inline std::string shortened_path(const std::string& game_path) {
            size_t last_slash = game_path.find_last_of('/');
            size_t last_backslash = game_path.find_last_of('\\');
            size_t short_start = last_slash != std::string::npos
                ? last_slash + 1
                : last_backslash != std::string::npos
                    ? last_backslash + 1
                    : 0;
            std::string short_name = game_path.substr(short_start);
            return short_name.size() > max_game_name_len
                ? short_name.substr(0, max_game_name_len) + "..."
                : short_name;
        }

    };

}