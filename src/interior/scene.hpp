
#pragma once

#include <engine/scene.hpp>
#include <engine/window.hpp>
#include "../player.hpp"
#include "interiors.hpp"

namespace houseofatmos::interior {

    struct Scene: engine::Scene {
        
        std::shared_ptr<Scene> outside;
        const Interior& interior;

        Renderer renderer;
        Player player;

        Scene(const Interior& interior);
        void load_resources();

        bool valid_player_position(const AbsCollider& player_coll);
        void update(engine::Window& window) override;
        
        void render_geometry(engine::Window& window, bool render_all_rooms);
        void render(engine::Window& window) override;

    };

}