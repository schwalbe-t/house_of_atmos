
// auto new_scene = std::make_shared<world::Scene>(after);

// window.set_scene(new_scene);


#include "tutorial.hpp"
#include "common.hpp"
#include "../interior/scene.hpp"

namespace houseofatmos::tutorial {

    static const f64 maid_v_pitch = 2.3;
    static const f64 maid_v_speed = 3.3;

    static Cutscene create_cutscene(std::shared_ptr<interior::Scene> scene) {
        // this is only legal because the previous scene already loads this :/
        const engine::Localization* local 
            = &scene->get(scene->world->settings.localization());
        auto force_player_sit = [scene](engine::Window& window) {
            (void) window;
            scene->created_interactables.clear();
            scene->player.character.position = { 0, 0.8, 10.3 };
            scene->player.character.action = Character::Action(
                (u64) human::Animation::Sit, INFINITY
            );
            scene->player.character.face_in_direction({ 0, 0, -1 });
        };
        scene->characters.clear();
        scene->characters.push_back({
            Character(
                &human::female, &human::maid, { 5, 0, 0 },
                (u64) human::Animation::Stand
            ),
            [](auto& c, auto& s, auto& w) { (void) c; (void) s; (void) w; }
        });
        Character* maid = &scene->characters.back().first;
        maid->face_in_direction({ 0, 0, 1 });
        return {
            await_delay(force_player_sit, 2.0),
            move_character_to(maid, { 0, 0, 0 }, 3.0),
            await_character_animation(
                scene, force_player_sit, maid, (u64) human::Animation::Stand
            ),
            move_character_to(maid, { 0, 0, 2 }, 3.0),
            await_character_animation(
                scene, force_player_sit, maid, (u64) human::Animation::Stand
            ),
            say_dialogue(
                scene, local,
                "dialogue_maid_name", "dialogue_tutorial_4_maid_0",
                voice::voiced, maid_v_pitch, maid_v_speed
            ),
            await_dialogue_end(scene, force_player_sit),
            move_character_to(maid, { 0, 0, 6 }, 3.0),
            await_character_animation(
                scene, force_player_sit, maid, (u64) human::Animation::Stand
            ),
            say_dialogue(
                scene, local,
                "dialogue_maid_name", "dialogue_tutorial_4_maid_1",
                voice::voiced, maid_v_pitch, maid_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_maid_name", "dialogue_tutorial_4_maid_2",
                voice::voiced, maid_v_pitch, maid_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_maid_name", "dialogue_tutorial_4_maid_3",
                voice::voiced, maid_v_pitch, maid_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_maid_name", "dialogue_tutorial_4_maid_4",
                voice::voiced, maid_v_pitch, maid_v_speed
            ),
            await_dialogue_end(
                scene, force_player_sit,
                [scene](engine::Window& window) {
                    scene->add_exit_interaction(window);
                    scene->add_interactions(window);
                    scene->characters.clear();
                    scene->player.character.position = { -1.5, 0, 10.5 };
                    scene->player.character.action = Character::Action(
                        (u64) human::Animation::Stand, 0.0
                    );
                    scene->player.character.face_in_direction({ -1, 0, 0 });
                }
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name", "dialogue_tutorial_4_prompt_5",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name", "dialogue_tutorial_4_prompt_6",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name", "dialogue_tutorial_4_prompt_7",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene)
        };
    }

    

    std::shared_ptr<engine::Scene> create_discovery_scene(
        std::shared_ptr<world::World> world_after
    ) {
        auto scene = std::make_shared<interior::Scene>(
            interior::mansion,
            std::shared_ptr<world::World>(world_after), 
            std::make_shared<world::Scene>(world_after)
        );
        scene->cutscene.append(create_cutscene(scene));
        return scene;
    }

}