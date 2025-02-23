
#include "tutorial.hpp"
#include "../world/scene.hpp"

namespace houseofatmos::tutorial {

    static const u64 seed = 1740334904;

    static world::World create_world(Settings&& settings) {
        auto world = world::World(std::move(settings), 75, 75);
        world.saving_allowed = false;
        world.generate_map(seed);
        world.player.character.type = &human::toddler;
        return world;
    }

    static Cutscene create_cutscene(
        std::shared_ptr<world::Scene> scene,
        std::shared_ptr<world::World>&& world_after
    ) {
        return {
            // Generating the final game world takes up a single frame,
            // which causes delta time to become massive in the following.
            // This first section just makes it so we essentially ignore
            // the first frame.
            Cutscene::Section(
                0.0, 
                [](engine::Window& window) { (void) window; },
                [](engine::Window& window) { (void) window; }
            ),
            Cutscene::Section(
                20.0,
                [scene](engine::Window& window) {
                    (void) window;
                    scene->world->personal_horse.pos 
                        = Vec<3>(INFINITY, 0, INFINITY);
                    scene->interactables.forget_all();
                },
                [world_after = std::move(world_after)](engine::Window& window) {
                    window.set_scene(
                        std::make_shared<world::Scene>(world_after)
                    );
                }
            )
        };
    }

    std::shared_ptr<engine::Scene> create_scene(
        std::shared_ptr<world::World>&& world_after
    ) {
        auto world = std::make_shared<world::World>(
            create_world(Settings(world_after->settings))
        );
        auto scene = std::make_shared<world::Scene>(std::move(world));
        scene->cutscene.append(create_cutscene(scene, std::move(world_after)));
        return scene;
    }

}