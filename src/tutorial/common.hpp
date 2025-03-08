
#include "../world/scene.hpp"

namespace houseofatmos::tutorial {

    static inline const f64 father_v_pitch = 1.8;
    static inline const f64 father_v_speed = 3.0;
    static inline const f64 prompt_v_pitch = 2.0;
    static inline const f64 prompt_v_speed = 3.0;


    void for_line(
        u64 min_x, u64 min_z, u64 max_x, u64 max_z,
        std::function<void (u64, u64)> action
    );

    std::function<void (u64, u64)> remove_foliage_from(world::Terrain& terrain);

    std::function<void (u64, u64)> set_path_at(world::Terrain& terrain);



    template<typename S>
    Cutscene::Section await_condition(
        std::shared_ptr<S> scene,
        std::function<void (engine::Window&)> on_update,
        std::function<bool (engine::Window&)> cond,
        std::function<void (engine::Window&)> on_end 
            = [](auto& w) { (void) w; }
    ) {
        return Cutscene::Section(
            INFINITY,
            [scene, on_update, cond](engine::Window& window) {
                on_update(window);
                if(cond(window)) {
                    scene->cutscene.advance();
                }
            },
            std::move(on_end)
        );
    }

    template<typename S>
    Cutscene::Section await_distance_to_point(
        std::shared_ptr<S> scene,
        std::function<void (engine::Window&)> on_update,
        Vec<3> point, f64 max_distance,
        std::function<void (engine::Window&)> on_end 
            = [](auto& w) { (void) w; }
    ) {
        return Cutscene::Section(
            INFINITY,
            [scene, on_update, point, max_distance](engine::Window& window) {
                on_update(window);
                Vec<3> offset = point
                    - scene->world->player.character.position;
                if(offset.len() <= max_distance) {
                    scene->cutscene.advance();
                }
            },
            std::move(on_end)
        );
    }

    template<typename S>
    Cutscene::Section say_dialogue(
        std::shared_ptr<S> scene, const engine::Localization* local,
        std::string name_key, std::string text_key,
        const Dialogue::Voice& voice, f64 v_pitch, f64 v_speed
    ) {
        return Cutscene::Section(
            0.0,
            [](auto& w) { (void) w; },
            [
                scene, local, name_key, text_key,
                voice = &voice, v_pitch, v_speed
            ](auto& w) {
                (void) w;
                scene->dialogues.say(Dialogue(
                    std::string(local->text(name_key)),
                    std::string(local->text(text_key)),
                    voice, v_pitch, v_speed,
                    { 0, 0, 0 }, INFINITY
                ));
            }
        );
    }

    template<typename S>
    Cutscene::Section await_dialogue_end(
        std::shared_ptr<S> scene,
        std::function<void (engine::Window&)> on_update 
            = [](auto& w) { (void) w; },
        std::function<void (engine::Window&)> on_end 
            = [](auto& w) { (void) w; }
    ) {
        return Cutscene::Section(
            INFINITY,
            [scene, on_update](engine::Window& window) {
                on_update(window);
                if(scene->dialogues.is_empty()) {
                    scene->cutscene.advance();
                }
            },
            std::move(on_end)
        );
    }

    template<typename S>
    Cutscene::Section await_distance_to_character(
        std::shared_ptr<S> scene,
        std::function<void (engine::Window&)> on_update,
        Character* character, f64 max_distance,
        std::function<void (engine::Window&)> on_end 
            = [](auto& w) { (void) w; }
    ) {
        return Cutscene::Section(
            INFINITY,
            [scene, on_update, character, max_distance](engine::Window& window) {
                on_update(window);
                Vec<3> offset = character->position
                    - scene->world->player.character.position;
                if(offset.len() <= max_distance) {
                    scene->cutscene.advance();
                }
            },
            std::move(on_end)
        );
    }

    Cutscene::Section move_character_to(
        Character* character, Vec<3> dest, f64 speed
    );

    template<typename S>
    Cutscene::Section await_character_animation(
        std::shared_ptr<S> scene,
        std::function<void (engine::Window&)> on_update,
        Character* character, u64 animation_id,
        std::function<void (engine::Window&)> on_end 
            = [](auto& w) { (void) w; }
    ) {
        return Cutscene::Section(
            INFINITY,
            [scene, on_update, character, animation_id](engine::Window& window) {
                on_update(window);
                if(character->action.animation_id == animation_id) {
                    scene->cutscene.advance();
                }
            },
            std::move(on_end)
        );
    }

    Cutscene::Section await_delay(
        std::function<void (engine::Window&)> on_update,
        f64 delayed_seconds,
        std::function<void (engine::Window&)> on_end 
            = [](auto& w) { (void) w; }
    );

}