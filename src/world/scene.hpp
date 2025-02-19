
#pragma once

#include <engine/localization.hpp>
#include <engine/arena.hpp>
#include <engine/window.hpp>
#include <engine/model.hpp>
#include <engine/ui.hpp>
#include "world.hpp"
#include "../interactable.hpp"
#include "../ui_const.hpp"
#include "../audio_const.hpp"
#include "../renderer.hpp"
#include "../dialogue.hpp"
#include "actionmode.hpp"
#include "terrainmap.hpp"

namespace houseofatmos::world {

    namespace ui = houseofatmos::engine::ui;


    struct Scene: engine::Scene {

        static inline const f64 min_camera_dist = 15.0;
        static inline const f64 max_camera_dist = 60.0;

        std::shared_ptr<World> world;

        Renderer renderer;
        DirectionalLight* sun;
        Interactables interactables;
        std::vector<Character> characters;

        f64 camera_distance = min_camera_dist;
        std::unique_ptr<ActionMode> action_mode = nullptr;
        ui::Manager ui = ui::Manager(ui_const::unit_size_fract);
        ui::Element* coin_counter = nullptr;
        TerrainMap terrain_map;
        Toasts toasts;
        DialogueManager dialogues;

        Scene(std::shared_ptr<World> world);
        void load_resources();
        static DirectionalLight create_sun(const Vec<3>& focus_point);
        static void configure_renderer(Renderer& renderer, u64 view_distance);

        f64 draw_distance_units() const {
            return this->world->settings.view_distance 
                * World::tiles_per_chunk * World::units_per_tile;
        }

        void update(engine::Window& window) override;

        void render_geometry(const engine::Window& window);
        void render(engine::Window& window) override;

    };

}