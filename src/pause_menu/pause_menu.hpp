
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
#include "../save_info.hpp"

namespace houseofatmos {

    namespace ui = houseofatmos::engine::ui;


    struct PauseMenu: engine::Scene {

        static inline const engine::Shader::LoadArgs blur_shader = {
            "res/shaders/blur_vert.glsl", "res/shaders/blur_frag.glsl"
        };


        SaveInfo save_info;
        engine::Localization::LoadArgs local_ref;
        std::shared_ptr<Scene> previous;
        const engine::Texture& last_frame;

        std::optional<engine::Texture> background = std::nullopt;
        ui::Manager ui = ui::Manager(ui_const::unit_size_fract);
        Toasts toasts = Toasts(this->local_ref);

        PauseMenu(
            SaveInfo save_info,
            std::shared_ptr<Scene> previous, const engine::Texture& last_frame
        );

        void refresh_ui_elements(engine::Window& window);
        void save_game(engine::Window& window, bool is_new_save = false);

        void update(engine::Window& window) override;
        void render(engine::Window& window) override;

    };

}