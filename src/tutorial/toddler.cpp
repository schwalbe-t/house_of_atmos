
#include "tutorial.hpp"
#include "../world/scene.hpp"

namespace houseofatmos::tutorial {

    static const u64 seed = 1740334904;

    static world::World create_world(Settings&& settings) {
        auto world = world::World(std::move(settings), 32, 32);
        world.terrain.generate_foliage();
        world.saving_allowed = false;
        world.player.character.type = &human::toddler;
        world.player.character.position = Vec<3>(16, 0, 16)
            * world.terrain.units_per_tile();
        world.balance.coins = 0;
        return world;
    }

    static Cutscene create_cutscene(
        std::shared_ptr<world::Scene> scene,
        std::shared_ptr<world::World>&& world_after
    ) {
        return {
            Cutscene::Section(
                INFINITY,
                [scene](engine::Window& window) {
                    (void) window;
                    scene->action_mode.remove_mode();
                    scene->world->personal_horse.pos 
                        = Vec<3>(INFINITY, 0, INFINITY);
                    if(window.was_pressed(engine::Key::Tab)) {
                        scene->cutscene.advance(window);
                    }
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