
#pragma once

#include <engine/scene.hpp>
#include <engine/window.hpp>
#include <engine/ui.hpp>
#include <engine/localization.hpp>
#include "../settings.hpp"
#include "../ui_const.hpp"
#include "../audio_const.hpp"
#include "../renderer.hpp"
#include "../world/terrain.hpp"

namespace houseofatmos {

    namespace ui = houseofatmos::engine::ui;


    struct MainMenu: engine::Scene {

        static const inline ui::Background title_sprite
            = MAKE_HOA_UI_ICON(Vec<2>(16, 240), Vec<2>(71, 40));

        static inline const engine::Shader::LoadArgs blur_shader = {
            "res/shaders/blur_vert.glsl", "res/shaders/blur_frag.glsl"
        };


        Settings settings;
        engine::Localization::LoadArgs local_ref;

        static inline const u64 units_per_tile = 5;
        static inline const u64 tiles_per_chunk = 5;
        static inline const i64 draw_distance_ch = 2;

        Renderer renderer;
        engine::Texture background = engine::Texture(16, 16);
        world::Terrain terrain = world::Terrain(
            32, 32, MainMenu::units_per_tile, MainMenu::tiles_per_chunk
        );
        ui::Manager ui = ui::Manager(0.0);

        std::function<void ()> before_next_frame = []() {};

        MainMenu(Settings&& settings);
        void load_resources();
        
        void load_game_from(
            const std::string& path, 
            const engine::Localization& local, engine::Window& window 
        );

        void show_title_screen(
            const engine::Localization& local, engine::Window& window
        );
        void show_loading_screen(const engine::Localization& local);
        void show_language_selection(engine::Window& window);
        void show_settings(
            const engine::Localization& local, engine::Window& window
        );
        void show_message(
            std::string_view local_text, 
            const engine::Localization& local, engine::Window& window
        );

        void update(engine::Window& window) override;
        void render(engine::Window& window) override;
        void render_background(engine::Window& window);

    };

}