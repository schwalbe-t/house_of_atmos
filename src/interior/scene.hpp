
#pragma once

#include <engine/scene.hpp>
#include <engine/window.hpp>
#include "../player.hpp"
#include "../interactable.hpp"
#include "../save_info.hpp"
#include "../audio_const.hpp"
#include "interiors.hpp"

namespace houseofatmos::interior {

    namespace ui = houseofatmos::engine::ui;


    struct Scene: engine::Scene {
        
        SaveInfo save_info;
        std::shared_ptr<engine::Scene> outside;

        const Interior& interior;

        Renderer renderer;
        Player player;
        ui::Manager ui = ui::Manager(ui_const::unit_size_fract);
        Interactables interactables;
        std::shared_ptr<Interactable> exit_interactable;

        Scene(
            const Interior& interior, 
            SaveInfo save_info,
            std::shared_ptr<engine::Scene> outside
        );
        void load_resources();

        bool valid_player_position(const AbsCollider& player_coll);
        void update(engine::Window& window) override;
        
        void render_geometry(bool render_all_rooms);
        void render(engine::Window& window) override;

    };

}