
#pragma once

#include <engine/scene.hpp>
#include <engine/window.hpp>
#include "../interactable.hpp"
#include "../audio_const.hpp"
#include "../ui_const.hpp"
#include "../player.hpp"
#include "../toasts.hpp"
#include "interiors.hpp"

namespace houseofatmos::world {

    struct World;

}

namespace houseofatmos::interior {

    namespace ui = houseofatmos::engine::ui;


    struct Scene: engine::Scene {
        
        std::shared_ptr<world::World> world;
        std::shared_ptr<engine::Scene> outside;

        const Interior& interior;

        Renderer renderer;
        Player player;
        ui::Manager ui = ui::Manager(ui_const::unit_size_fract);
        ui::Element* coin_counter = nullptr;
        Toasts toasts;
        Interactables interactables;
        std::shared_ptr<Interactable> exit_interactable;

        Scene(
            const Interior& interior, 
            std::shared_ptr<world::World>&& world,
            std::shared_ptr<engine::Scene>&& outside
        );
        void load_resources();

        bool valid_player_position(const AbsCollider& player_coll);
        void update(engine::Window& window) override;
        
        void render_geometry(const engine::Window& window, bool render_all_rooms);
        void render(engine::Window& window) override;

    };

}