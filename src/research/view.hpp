
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

namespace houseofatmos::research {

    namespace ui = houseofatmos::engine::ui;


    struct View: engine::Scene {

        static inline const engine::Shader::LoadArgs blur_shader = {
            "res/shaders/blur_vert.glsl", "res/shaders/blur_frag.glsl"
        };

        struct Anchor {
            Vec<2> cursor_pos;
            Vec<2> view_offset;
        };


        std::shared_ptr<world::World> world;
        std::shared_ptr<Scene> previous;
        const engine::Texture& last_frame;

        engine::Texture black_backdrop = engine::Texture(16, 16);
        std::optional<engine::Texture> background = std::nullopt;
        ui::Manager ui = ui::Manager(ui_const::unit_size_fract);
        ui::Element* view_root = nullptr;
        f64 view_update_timer = 0.0;
        Toasts toasts;
        std::optional<Anchor> view_anchor = std::nullopt;
        Vec<2> view_offset;

        View(
            std::shared_ptr<world::World>&& world,
            std::shared_ptr<Scene>&& previous, 
            const engine::Texture& last_frame
        );

        void init_ui();
        void update_view(const engine::Window& window);

        void update(engine::Window& window) override;
        void render(engine::Window& window) override;

    };

}