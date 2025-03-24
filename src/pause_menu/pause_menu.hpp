
#pragma once

#include <engine/scene.hpp>
#include <engine/window.hpp>
#include <engine/ui.hpp>
#include <engine/localization.hpp>
#include <engine/arena.hpp>
#include "../settings.hpp"
#include "../ui_const.hpp"
#include "../audio_const.hpp"
#include "../toasts.hpp"
#include "../world/world.hpp"

namespace houseofatmos {

    namespace ui = houseofatmos::engine::ui;


    struct PauseMenu: engine::Scene {

        static inline const engine::Shader::LoadArgs blur_shader = {
            "res/shaders/blur_vert.glsl", "res/shaders/blur_frag.glsl"
        };


        std::shared_ptr<world::World> world;
        std::shared_ptr<Scene> previous;
        const engine::Texture& last_frame;

        std::optional<engine::Texture> background = std::nullopt;
        ui::Manager ui = ui::Manager(0.0);
        Toasts toasts;

        PauseMenu(
            std::shared_ptr<world::World>&& world,
            std::shared_ptr<Scene>&& previous, 
            const engine::Texture& last_frame
        );

        void show_root_menu(engine::Window& window);
        void show_settings(engine::Window& window);
        void save_game(engine::Window& window, bool is_new_save = false);

        void update(engine::Window& window) override;
        void render(engine::Window& window) override;

    };

}