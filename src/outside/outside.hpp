
#pragma once

#include <engine/localization.hpp>
#include <engine/arena.hpp>
#include <engine/window.hpp>
#include <engine/model.hpp>
#include <engine/ui.hpp>
#include "../interactable.hpp"
#include "../settings.hpp"
#include "../ui_const.hpp"
#include "../renderer.hpp"
#include "../player.hpp"
#include "terrain.hpp"
#include "actionmode.hpp"
#include "carriage.hpp"
#include "terrainmap.hpp"
#include "personal_horse.hpp"

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
            PersonalHorse::Serialized personal_horse;
        };


        static inline const u64 units_per_tile = 5;
        static inline const u64 tiles_per_chunk = 5;
        static inline const i64 draw_distance_ch = 2;
        static inline const i64 draw_distance_un    
            = draw_distance_ch * tiles_per_chunk * units_per_tile;

        static inline const f64 min_camera_dist = 15.0;
        static inline const f64 max_camera_dist = 60.0;

        Settings settings;
        engine::Localization::LoadArgs local;
        std::string save_path;

        Renderer renderer;
        DirectionalLight* sun;
        Terrain terrain = Terrain(
            256, 256, draw_distance_ch, units_per_tile, tiles_per_chunk
        );
        ComplexBank complexes;
        Player player;
        Balance balance;
        CarriageManager carriages;
        Interactables interactables = Interactables();
        PersonalHorse personal_horse = PersonalHorse(
            Vec<3>(), &this->player, &this->interactables
        );

        f64 camera_distance = min_camera_dist;
        std::unique_ptr<ActionMode> action_mode;
        ui::Element* coins_elem = nullptr;
        ui::Manager ui = ui::Manager(ui_const::unit_size_fract);
        TerrainMap terrain_map = TerrainMap(
            this->local, this->terrain, this->complexes, this->player, 
            this->carriages, this->personal_horse, this->ui
        );
        Toasts toasts = Toasts(this->local);

        Outside(Settings&& settings);
        Outside(Settings&& settings, const engine::Arena& buffer);
        void load_resources();
        static DirectionalLight create_sun();
        static void configure_renderer(Renderer& renderer);

        void update(engine::Window& window) override;
        void render(engine::Window& window) override;

        engine::Arena serialize() const;

    };

}