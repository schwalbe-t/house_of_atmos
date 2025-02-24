
#include "tutorial.hpp"
#include "common.hpp"

namespace houseofatmos::tutorial {

    static world::World create_world(Settings&& settings) {
        auto world = world::World(std::move(settings), 48, 48);
        world.terrain.set_area_elevation(0, 0, 48, 48, 3);
        world.terrain.set_area_elevation(30, 0, 36, 48, 1);
        world.terrain.set_area_elevation(31, 0, 35, 48, 0);
        world.terrain.set_area_elevation(32, 0, 34, 48, -1);
        world.terrain.set_area_elevation(33, 0, 33, 48, -2);
        world.terrain.generate_foliage();
        world.terrain.place_building(world::Building::Mansion, 12, 22);
        world.terrain.remove_foliage_at(13, 26);
        for_line(29, 18, 36, 18, remove_foliage_from(world.terrain));
        world.terrain.bridges.push_back((world::Bridge) {
            world::Bridge::Stone, 29, 18, 36, 18, 3
        });
        for_line(13, 23, 13, 25, set_path_at(world.terrain));
        for_line(10, 25, 20, 25, set_path_at(world.terrain));
        for_line(20, 25, 20, 31, set_path_at(world.terrain));
        for_line(24, 31, 28, 31, set_path_at(world.terrain));
        for_line(28, 18, 28, 31, set_path_at(world.terrain));
        for_line(37, 18, 41, 18, set_path_at(world.terrain));
        world.saving_allowed = false;
        world.player.character.type = &human::toddler;
        world.player.character.position = Vec<3>(13.5, 0, 24.5)
            * world.terrain.units_per_tile();
        world.balance.coins = 0;
        return world;
    }



    static std::function<void (engine::Window&)> base_restrictions_over(
        std::shared_ptr<world::Scene> scene, 
        std::shared_ptr<world::World> after
    ) {
        return [scene, after](engine::Window& window) {
            (void) window;
            scene->action_mode.remove_mode();
            scene->world->personal_horse.pos 
                = Vec<3>(INFINITY, 0, INFINITY);
            if(window.was_pressed(engine::Key::Tab)) {
                window.set_scene(std::make_shared<world::Scene>(after));
            }
        };
    }
    
    static Cutscene::Section await_advance(
        std::shared_ptr<world::Scene> scene,
        std::shared_ptr<world::World> after
    ) {
        return Cutscene::Section(
            INFINITY,
            base_restrictions_over(scene, after), 
            [](auto& w) { (void) w; }
        );
    }

    static Cutscene::Section await_condition(
        std::shared_ptr<world::Scene> scene,
        std::shared_ptr<world::World> after,
        std::function<bool (engine::Window&)> cond,
        std::function<void (engine::Window&)> handler 
            = [](auto& w) { (void) w; }
    ) {
        return Cutscene::Section(
            INFINITY,
            [
                scene, cond, base_restr = base_restrictions_over(scene, after)
            ](engine::Window& window) {
                base_restr(window);
                if(cond(window)) {
                    scene->cutscene.advance();
                }
            },
            std::move(handler)
        );
    }

    static std::function<void (engine::Window&)> say_dialogue(
        std::shared_ptr<world::Scene> scene, const engine::Localization* local,
        std::string name_key, std::string text_key,
        const Dialogue::Voice& voice, f64 v_pitch, f64 v_speed,
        std::function<void ()> on_end
    ) {
        return [
            scene, local, name_key, text_key,
            voice = &voice, v_pitch, v_speed, on_end
        ](auto& w) {
            (void) w;
            scene->dialogues.say(Dialogue(
                std::string(local->text(name_key)),
                std::string(local->text(text_key)),
                voice, v_pitch, v_speed,
                { 0, 0, 0 }, INFINITY,
                std::function(on_end)
            ));
        };
    }

    static std::function<bool (engine::Window&)> player_close_to_father(
        std::shared_ptr<world::Scene> scene, Character* father
    ) {
        return [scene, father](engine::Window& window) {
            (void) window;
            if(father->action.animation_id == (u64) human::Animation::Walk) {
                return false;
            }
            Vec<3> offset = father->position
                - scene->world->player.character.position;
            return offset.len() <= 3.0;
        };
    }

    static std::function<void (engine::Window&)> move_to_dest(
        Character* character, Vec<3> dest, f64 speed
    ) {
        return [character, dest, speed](engine::Window& window) {
            (void) window;
            Vec<3> offset = dest - character->position;
            f64 duration = offset.len() / speed;
            character->action = Character::Action(
                (u64) human::Animation::Walk, dest, duration
            );
            character->face_in_direction(offset);
        };
    }

    static Cutscene create_cutscene(
        std::shared_ptr<world::Scene> scene,
        std::shared_ptr<world::World> after
    ) {
        // this is only legal because the main menu already loads this :/
        const engine::Localization* local = &scene
            ->get<engine::Localization>(scene->world->settings.localization());
        scene->characters.push_back(Character(
            &human::male, &human::count, { 0, 0, 0 },
            (u64) human::Animation::Stand
        ));
        Character* father = &scene->characters.back();
        father->position = Vec<3>(13.5, 0, 25.5) 
            * scene->world->terrain.units_per_tile()
            + Vec<3>(0.0, 3.0, 0.0);
        father->face_in_direction({ 0, 0, -1 });
        return {
            // Long load times make deltatime spike in the first frame,
            // which skips most of the first section.
            // This is here so that doesn't cause a problem :)
            Cutscene::Section(
                0.0, [](auto& w) { (void) w; },  [](auto& w) { (void) w; }
            ),
            Cutscene::Section(
                1.0,
                base_restrictions_over(scene, after),
                say_dialogue(
                    scene, local, 
                    "dialogue_tutorial_prompt_name", 
                    "dialogue_tutorial_0_prompt_0",
                    voice::popped, prompt_v_pitch, prompt_v_speed,
                    [scene]() { scene->cutscene.advance(); }
                )
            ),
            // wait for 'dialogue_tutorial_0_prompt_0' to end 
            Cutscene::Section(
                INFINITY,
                base_restrictions_over(scene, after),
                [](auto& w) { (void) w; }
            ),
            Cutscene::Section(
                1.0,
                base_restrictions_over(scene, after),
                say_dialogue(
                    scene, local, 
                    "dialogue_tutorial_father_name", 
                    "dialogue_tutorial_0_father_1",
                    voice::voiced, father_v_pitch, father_v_speed,
                    [scene]() { scene->cutscene.advance(); }
                )
            ),
            // wait for 'dialogue_tutorial_0_father_1' to end 
            Cutscene::Section(
                INFINITY, 
                base_restrictions_over(scene, after), 
                say_dialogue(
                    scene, local, 
                    "dialogue_tutorial_prompt_name", 
                    "dialogue_tutorial_0_prompt_2",
                    voice::popped, prompt_v_pitch, prompt_v_speed,
                    [scene]() { scene->cutscene.advance(); }
                )
            ),
            // wait for 'dialogue_tutorial_0_prompt_2' to end 
            await_advance(scene, after),
            // repeatedly wait for player to be near, then move along again
            await_condition(
                scene, after, 
                player_close_to_father(scene, father),
                move_to_dest(
                    father, 
                    Vec<3>(15.5, 0, 25.5) * scene->world->terrain.units_per_tile()
                        + Vec<3>(0.0, 3.0, 0.0),
                    2.0
                )
            ),
            await_condition(
                scene, after, 
                player_close_to_father(scene, father),
                move_to_dest(
                    father, 
                    Vec<3>(17.5, 0, 25.5) * scene->world->terrain.units_per_tile()
                        + Vec<3>(0.0, 3.0, 0.0),
                    3.0
                )
            ),
            await_condition(
                scene, after, 
                player_close_to_father(scene, father),
                move_to_dest(
                    father, 
                    Vec<3>(20.5, 0, 25.5) * scene->world->terrain.units_per_tile()
                        + Vec<3>(0.0, 3.0, 0.0),
                    3.0
                )
            ),
            await_condition(
                scene, after, 
                player_close_to_father(scene, father),
                move_to_dest(
                    father, 
                    Vec<3>(20.5, 0, 28.5) * scene->world->terrain.units_per_tile()
                        + Vec<3>(0.0, 3.0, 0.0),
                    3.0
                )
            ),
            await_condition(
                scene, after, 
                player_close_to_father(scene, father),
                move_to_dest(
                    father, 
                    Vec<3>(20.5, 0, 31.5) * scene->world->terrain.units_per_tile()
                        + Vec<3>(0.0, 3.0, 0.0),
                    3.0
                )
            ),
            // wait for the walking sequence to end
            await_condition(
                scene, after, 
                player_close_to_father(scene, father),
                say_dialogue(
                    scene, local, 
                    "dialogue_tutorial_father_name", 
                    "dialogue_tutorial_0_father_3",
                    voice::voiced, father_v_pitch, father_v_speed,
                    [scene]() { scene->cutscene.advance(); }
                )
            ),
            // wait for 'dialogue_tutorial_0_father_3' to end
            Cutscene::Section(
                INFINITY, 
                base_restrictions_over(scene, after), 
                say_dialogue(
                    scene, local, 
                    "dialogue_tutorial_prompt_name", 
                    "dialogue_tutorial_0_prompt_4",
                    voice::popped, prompt_v_pitch, prompt_v_speed,
                    [scene]() { scene->cutscene.advance(); }
                )
            ),
            // wait for 'dialogue_tutorial_0_prompt_4' to end
            Cutscene::Section(
                INFINITY, 
                base_restrictions_over(scene, after),
                [](auto& w) { (void) w; }
            ),
            // wait for the player to zoom out the camera
            await_condition(
                scene, after,
                [scene](engine::Window& window) {
                    (void) window;
                    f64 zoom_range = world::Scene::max_camera_dist 
                        - world::Scene::min_camera_dist;
                    f64 required_zoom = world::Scene::min_camera_dist
                        + zoom_range * 0.75;
                    return scene->camera_distance >= required_zoom; 
                },
                say_dialogue(
                    scene, local, 
                    "dialogue_tutorial_father_name", 
                    "dialogue_tutorial_0_father_5",
                    voice::voiced, father_v_pitch, father_v_speed,
                    [scene]() { scene->cutscene.advance(); }
                )
            ),
            // wait for 'dialogue_tutorial_0_father_5' to end
            Cutscene::Section(
                INFINITY, 
                base_restrictions_over(scene, after),
                move_to_dest(
                    father, 
                    Vec<3>(24.5, 0, 31.5) * scene->world->terrain.units_per_tile()
                        + Vec<3>(0.0, 3.0, 0.0),
                    2.0
                )
            ),
            await_condition(
                scene, after, 
                player_close_to_father(scene, father),
                move_to_dest(
                    father, 
                    Vec<3>(28.5, 0, 31.5) * scene->world->terrain.units_per_tile()
                        + Vec<3>(0.0, 3.0, 0.0),
                    3.0
                )
            ),
            await_condition(
                scene, after, 
                player_close_to_father(scene, father),
                move_to_dest(
                    father, 
                    Vec<3>(28.5, 0, 25.5) * scene->world->terrain.units_per_tile()
                        + Vec<3>(0.0, 3.0, 0.0),
                    3.0
                )
            ),
            await_condition(
                scene, after, 
                player_close_to_father(scene, father),
                move_to_dest(
                    father, 
                    Vec<3>(28.5, 0, 18.5) * scene->world->terrain.units_per_tile()
                        + Vec<3>(0.0, 3.0, 0.0),
                    3.0
                )
            ),
            await_condition(
                scene, after, 
                player_close_to_father(scene, father),
                move_to_dest(
                    father, 
                    Vec<3>(33, 0, 18.5) * scene->world->terrain.units_per_tile()
                        + Vec<3>(0.0, 3.0, 0.0),
                    3.0
                )
            ),
            await_condition(
                scene, after, 
                player_close_to_father(scene, father),
                move_to_dest(
                    father, 
                    Vec<3>(33, 0, 18.85) * scene->world->terrain.units_per_tile()
                        + Vec<3>(0.0, 3.0, 0.0),
                    3.0
                )
            ),
            // wait for sequence to end
            await_condition(
                scene, after, 
                player_close_to_father(scene, father),
                say_dialogue(
                    scene, local, 
                    "dialogue_tutorial_father_name", 
                    "dialogue_tutorial_0_father_6",
                    voice::voiced, father_v_pitch, father_v_speed,
                    [scene]() { scene->cutscene.advance(); }
                )
            ),
            // wait for 'dialogue_tutorial_0_father_6' to end
            Cutscene::Section(
                INFINITY, 
                base_restrictions_over(scene, after),
                [](auto& w) { (void) w; }
            ),
            Cutscene::Section(
                3.0, 
                base_restrictions_over(scene, after),
                [after](engine::Window& window) {
                    window.set_scene(create_grownup_scene(after));
                }
            )
        };
    }



    std::shared_ptr<engine::Scene> create_toddler_scene(
        std::shared_ptr<world::World> world_after
    ) {
        auto world = std::make_shared<world::World>(
            create_world(Settings(world_after->settings))
        );
        auto scene = std::make_shared<world::Scene>(std::move(world));
        scene->cutscene.append(create_cutscene(scene, world_after));
        return scene;
    }

}