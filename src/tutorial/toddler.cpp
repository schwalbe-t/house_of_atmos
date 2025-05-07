
#include "tutorial.hpp"
#include "common.hpp"

namespace houseofatmos::tutorial {

    static std::shared_ptr<world::World> create_world(Settings&& settings) {
        auto world = std::make_shared<world::World>(
            std::move(settings), 48, 48
        );
        world->terrain.set_area_elevation(0, 0, 48, 48, 3);
        world->terrain.set_area_elevation(30, 0, 36, 48, 1);
        world->terrain.set_area_elevation(31, 0, 35, 48, 0);
        world->terrain.set_area_elevation(32, 0, 34, 48, -1);
        world->terrain.set_area_elevation(33, 0, 33, 48, -2);
        world->terrain.generate_foliage();
        world->terrain.place_building(world::Building::Mansion, 12, 22);
        world->terrain.remove_foliage_at(13, 26);
        for_line(29, 18, 36, 18, remove_foliage_from(world->terrain));
        world->terrain.bridges.push_back((world::Bridge) {
            world::Bridge::Stone, 29, 18, 36, 18, 3
        });
        for_line(13, 23, 13, 25, set_path_at(world->terrain));
        for_line(10, 25, 20, 25, set_path_at(world->terrain));
        for_line(20, 25, 20, 31, set_path_at(world->terrain));
        for_line(24, 31, 28, 31, set_path_at(world->terrain));
        for_line(28, 18, 28, 31, set_path_at(world->terrain));
        for_line(37, 18, 41, 18, set_path_at(world->terrain));
        world->saving_allowed = false;
        world->player.character.type = &human::toddler;
        world->player.character.position = Vec<3>(13.5, 0, 24.5)
            * world->terrain.units_per_tile();
        world->balance.set_coins_silent(0);
        return world;
    }



    static Cutscene create_cutscene(
        std::shared_ptr<world::Scene> scene,
        std::shared_ptr<world::World> after
    ) {
        // this is only legal because the main menu already loads this :/
        const engine::Localization* local
            = &scene->get(scene->world->settings.localization());
        scene->characters.push_back(Character(
            &human::male, &human::count, { 0, 0, 0 },
            (u64) human::Animation::Stand
        ));
        Character* father = &scene->characters.back();
        father->position = Vec<3>(13.5, 0, 25.5) 
            * scene->world->terrain.units_per_tile()
            + Vec<3>(0.0, 3.0, 0.0);
        father->face_in_direction({ 0, 0, -1 });
        auto update_scene = [scene](engine::Window& window) {
            (void) window;
            scene->action_mode.remove_mode();
            scene->world->personal_horse.pos 
                = Vec<3>(INFINITY, 0, INFINITY);
            for(auto& chunk: scene->world->terrain.all_loaded_chunks()) {
                chunk.interactables.clear();
            }
        };
        auto await_father_standing = await_character_animation(
            scene, update_scene, father, (u64) human::Animation::Stand
        );
        return {
            // Long load times make deltatime spike in the first frame,
            // which skips most of the first section.
            // This 0 delay is here so that doesn't cause a problem :)
            await_delay(update_scene, 0.0),
            await_delay(update_scene, 2.5),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name", 
                "dialogue_tutorial_0_father_1",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            await_dialogue_end(scene, update_scene),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name", 
                "dialogue_tutorial_0_prompt_2",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, update_scene),
            // repeatedly wait for player to be near, then move along again
            await_distance_to_character(scene, update_scene, father, 3.0),
            move_character_to(
                father,
                Vec<3>(15.5, 0, 25.5) * scene->world->terrain.units_per_tile()
                    + Vec<3>(0.0, 3.0, 0.0),
                2.0
            ),
            await_father_standing,
            await_distance_to_character(scene, update_scene, father, 3.0),
            move_character_to(
                father,
                Vec<3>(17.5, 0, 25.5) * scene->world->terrain.units_per_tile()
                    + Vec<3>(0.0, 3.0, 0.0),
                3.0
            ),
            await_father_standing,
            await_distance_to_character(scene, update_scene, father, 3.0),
            move_character_to(
                father,
                Vec<3>(20.5, 0, 25.5) * scene->world->terrain.units_per_tile()
                    + Vec<3>(0.0, 3.0, 0.0),
                3.5
            ),
            await_father_standing,
            await_distance_to_character(scene, update_scene, father, 3.0),
            move_character_to(
                father,
                Vec<3>(20.5, 0, 28.5) * scene->world->terrain.units_per_tile()
                    + Vec<3>(0.0, 3.0, 0.0),
                3.5
            ),
            await_father_standing,
            await_distance_to_character(scene, update_scene, father, 3.0),
            move_character_to(
                father,
                Vec<3>(20.5, 0, 31.5) * scene->world->terrain.units_per_tile()
                    + Vec<3>(0.0, 3.0, 0.0),
                3.5
            ),
            await_father_standing,
            await_distance_to_character(scene, update_scene, father, 3.0),
            // dialogue sequence about zooming in / out
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name", 
                "dialogue_tutorial_0_father_3",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name", 
                "dialogue_tutorial_0_prompt_4",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            // wait for the player to zoom out the camera
            // (this may happen even during dialogue)
            await_condition(
                scene, update_scene,
                [scene](engine::Window& window) {
                    (void) window;
                    f64 zoom_range = world::Scene::max_camera_dist 
                        - world::Scene::min_camera_dist;
                    f64 required_zoom = world::Scene::min_camera_dist
                        + zoom_range * 0.75;
                    return scene->camera_distance >= required_zoom; 
                }
            ),
            await_dialogue_end(scene, update_scene),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name", 
                "dialogue_tutorial_0_father_5",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            await_dialogue_end(scene, update_scene),
            // continue walking sequence
            move_character_to(
                father,
                Vec<3>(24.5, 0, 31.5) * scene->world->terrain.units_per_tile()
                    + Vec<3>(0.0, 3.0, 0.0),
                2.5
            ),
            await_father_standing,
            await_distance_to_character(scene, update_scene, father, 3.0),
            move_character_to(
                father,
                Vec<3>(28.5, 0, 31.5) * scene->world->terrain.units_per_tile()
                    + Vec<3>(0.0, 3.0, 0.0),
                3.5
            ),
            await_father_standing,
            await_distance_to_character(scene, update_scene, father, 3.0),
            move_character_to(
                father,
                Vec<3>(28.5, 0, 25.5) * scene->world->terrain.units_per_tile()
                    + Vec<3>(0.0, 3.0, 0.0),
                3.5
            ),
            await_father_standing,
            await_distance_to_character(scene, update_scene, father, 3.0),
            move_character_to(
                father,
                Vec<3>(28.5, 0, 18.5) * scene->world->terrain.units_per_tile()
                    + Vec<3>(0.0, 3.0, 0.0),
                3.5
            ),
            await_father_standing,
            await_distance_to_character(scene, update_scene, father, 3.0),
            move_character_to(
                father,
                Vec<3>(33, 0, 18.5) * scene->world->terrain.units_per_tile()
                    + Vec<3>(0.0, 3.0, 0.0),
                3.5
            ),
            await_father_standing,
            await_distance_to_character(scene, update_scene, father, 3.0),
            move_character_to(
                father,
                Vec<3>(33, 0, 18.85) * scene->world->terrain.units_per_tile()
                    + Vec<3>(0.0, 3.0, 0.0),
                3.5
            ),
            await_father_standing,
            await_distance_to_character(scene, update_scene, father, 3.0),
            // final dialogue that ends the first tutorial
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name", 
                "dialogue_tutorial_0_father_6",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            await_dialogue_end(scene, update_scene),
            await_delay(
                update_scene, 3.0,
                [scene, after](engine::Window& window) {
                    after->settings = scene->world->settings;
                    window.set_scene(create_terraform_scene(after));
                }
            )
        };
    }



    std::shared_ptr<engine::Scene> create_toddler_scene(
        std::shared_ptr<world::World> world_after
    ) {
        auto world = create_world(Settings(world_after->settings));
        auto scene = std::make_shared<world::Scene>(world);
        scene->cutscene.append(create_cutscene(scene, world_after));
        return scene;
    }

}