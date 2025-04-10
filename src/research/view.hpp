
#pragma once

#include <engine/scene.hpp>
#include <engine/window.hpp>
#include <engine/ui.hpp>
#include <engine/localization.hpp>
#include <engine/arena.hpp>
#include "../settings.hpp"
#include "../audio_const.hpp"
#include "../toasts.hpp"
#include "../world/world.hpp"

namespace houseofatmos::research {

    namespace ui = houseofatmos::engine::ui;


    struct View: engine::Scene {

        static inline const engine::Texture::LoadArgs background_design = {
            "res/research.png"
        };


        std::shared_ptr<world::World> world;
        std::shared_ptr<Scene> previous;

        engine::Texture background = engine::Texture(16, 16);
        ui::Manager ui = ui::Manager(1.0 / 225.0);
        Toasts toasts;
        f64 view_update_timer = INFINITY;
        std::optional<Research::Condition> selected_cond = std::nullopt;

        View(
            std::shared_ptr<world::World>&& world,
            std::shared_ptr<Scene>&& previous
        );

        ui::Element build_research_tree();
        ui::Element build_condition_display(
            Research::Condition cond, const engine::Localization& local
        );
        void init_ui(engine::Window& window);

        void update(engine::Window& window) override;
        void render(engine::Window& window) override;

    };

}