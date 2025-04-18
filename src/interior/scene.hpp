
#pragma once

#include <engine/scene.hpp>
#include <engine/window.hpp>
#include "../interactable.hpp"
#include "../audio_const.hpp"
#include "../ui_const.hpp"
#include "../player.hpp"
#include "../toasts.hpp"
#include "../cutscene.hpp"
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

        Cutscene cutscene;

        Renderer renderer;
        Player player;
        ui::Manager ui = ui::Manager(0.0);
        ui::Element* coin_counter = nullptr;
        Toasts toasts;
        DialogueManager dialogues;
        Interactables interactables;
        std::vector<std::shared_ptr<Interactable>> created_interactables;
        std::vector<std::pair<Character, Interior::CharacterUpdate>> characters;

        Scene(
            const Interior& interior, 
            std::shared_ptr<world::World>&& world,
            std::shared_ptr<engine::Scene>&& outside
        );
        void load_resources();
        void add_exit_interaction(engine::Window& window);
        void add_interactions(engine::Window& window);
        void add_characters();
        static void configure_renderer(
            Renderer& renderer, 
            const Interior& interior, const Settings& settings
        );

        void init_ui(engine::Window& window);

        bool collides_with(const AbsCollider& coll);
        void update(engine::Window& window) override;
        
        void render_geometry(const engine::Window& window, bool render_all_rooms);
        void render(engine::Window& window) override;

    };

}