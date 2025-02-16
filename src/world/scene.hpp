
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
#include "actionmode.hpp"
#include "terrainmap.hpp"

namespace houseofatmos::world {

    namespace ui = houseofatmos::engine::ui;


    struct Scene: engine::Scene {

        static inline const i64 draw_distance_ch = 2;
        static inline const i64 draw_distance_un    
            = draw_distance_ch * World::tiles_per_chunk * World::units_per_tile;

        static inline const f64 min_camera_dist = 15.0;
        static inline const f64 max_camera_dist = 60.0;

        std::shared_ptr<World> world;

        Renderer renderer;
        DirectionalLight* sun;
        Interactables interactables;
        std::vector<Character> characters;

        f64 camera_distance = min_camera_dist;
        std::unique_ptr<ActionMode> action_mode;
        ui::Manager ui = ui::Manager(ui_const::unit_size_fract);
        ui::Element* coin_counter = nullptr;
        TerrainMap terrain_map;
        Toasts toasts;

        Scene(std::shared_ptr<World> world);
        void load_resources();
        static DirectionalLight create_sun(const Vec<3>& focus_point);
        static void configure_renderer(Renderer& renderer);

        void update(engine::Window& window) override;

        void render_geometry(const engine::Window& window);
        void render(engine::Window& window) override;

    };

}