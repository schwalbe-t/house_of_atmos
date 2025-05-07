
#include "tutorial.hpp"
#include "common.hpp"

namespace houseofatmos::tutorial {

    static std::shared_ptr<world::World> create_world(Settings&& settings) {
        auto world = std::make_shared<world::World>(
            std::move(settings), 64, 64
        );
        for(u64 x = 0; x < world->terrain.width_in_tiles(); x += 1) {
            for(u64 z = 0; z < world->terrain.height_in_tiles(); z += 1) {
                world->terrain.elevation_at(x, z) 
                    = world->terrain.rng.next_bool()? 11 : 10;
            }
        }
        world->terrain.generate_foliage();
        world->terrain.remove_foliage_at(13, 32);
        world->terrain.remove_foliage_at(13, 33);
        world->saving_allowed = false;
        world->player.character.type = &human::toddler;
        world->player.character.position = Vec<3>(13.75, 0, 32.5)
            * world->terrain.units_per_tile();
        world->balance.set_coins_silent(Balance::infinite_coins);
        return world;
    }



    static Cutscene create_cutscene(
        std::shared_ptr<world::Scene> scene,
        std::shared_ptr<world::World> after
    ) {
        // this is only legal because the previous scene already loads this :/
        const engine::Localization* local 
            = &scene->get(scene->world->settings.localization());
        scene->characters.push_back(Character(
            &human::male, &human::father, { 0, 0, 0 },
            (u64) human::Animation::Stand
        ));
        Character* father = &scene->characters.back();
        father->position = Vec<3>(13.25, 0, 32.5) 
            * scene->world->terrain.units_per_tile();
        father->position.y() = scene->world->terrain
            .elevation_at(father->position);
        father->face_in_direction({ 1, 0, 0 });
        auto update_scene = [scene](engine::Window& window) {
            (void) window;
            scene->world->personal_horse.pos = Vec<3>(INFINITY, 0, INFINITY);
            for(auto& chunk: scene->world->terrain.all_loaded_chunks()) {
                chunk.interactables.clear();
            }
        };
        return {
            await_delay(update_scene, 1.0),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_1_father_0",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_1_prompt_1",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_1_prompt_2",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_1_prompt_3",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_1_father_4",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            await_dialogue_end(scene, update_scene),
            await_condition(
                scene, update_scene,
                [scene](auto& w) {
                    (void) w;
                    i16 target = scene->world->terrain.elevation_at(13, 32);
                    bool is_flat = true;
                    for(u64 x = 12; x <= 14; x += 1) {
                        for(u64 z = 31; z <= 33; z += 1) {
                            i64 e = scene->world->terrain.elevation_at(x, z);
                            is_flat &= e == target;
                        }
                    }
                    return is_flat;
                }
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_1_father_5",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            await_dialogue_end(
                scene, update_scene,
                [scene, after](engine::Window& window) {
                    after->settings = scene->world->settings;
                    window.set_scene(create_buildings_scene(after));
                }
            ),
        };
    }

    

    std::shared_ptr<engine::Scene> create_terraform_scene(
        std::shared_ptr<world::World> world_after
    ) {
        auto world = create_world(Settings(world_after->settings));
        auto scene = std::make_shared<world::Scene>(world);
        scene->cutscene.append(create_cutscene(scene, world_after));
        return scene;
    }

}