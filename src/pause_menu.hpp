
#pragma once

#include <engine/scene.hpp>
#include <engine/window.hpp>
#include <engine/ui.hpp>
#include <engine/localization.hpp>
#include <engine/arena.hpp>
#include "ui_const.hpp"
#include "toasts.hpp"

namespace houseofatmos {

    namespace ui = houseofatmos::engine::ui;


    struct PauseMenu: engine::Scene {

        static inline const engine::Shader::LoadArgs blur_shader = {
            "res/shaders/blur_vert.glsl", "res/shaders/blur_frag.glsl"
        };


        std::shared_ptr<Scene> previous;
        const engine::Texture& last_frame;
        std::string& save_path;
        engine::Localization::LoadArgs local_ref;
        std::function<engine::Arena ()> serialize;

        std::optional<engine::Texture> background = std::nullopt;
        ui::Manager ui = ui::Manager(ui_const::unit_size_fract);
        Toasts toasts = Toasts(this->local_ref);

        PauseMenu(
            std::shared_ptr<Scene> previous, const engine::Texture& last_frame,
            std::string& save_path, engine::Localization::LoadArgs locale,
            std::function<engine::Arena ()>&& serialize
        );

        void refresh_ui_elements(engine::Window& window);
        void save_game(engine::Window& window, bool is_new_save = false);

        void update(engine::Window& window) override;
        void render(engine::Window& window) override;

    };

}