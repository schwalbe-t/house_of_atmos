
#include "interiors.hpp"
#include "scene.hpp"
#include "../world/world.hpp"
#include "../research/view.hpp"

namespace houseofatmos::interior {

    static void open_research_view(
        const std::shared_ptr<world::World>& world, 
        const Renderer& renderer, engine::Window& window
    ) {
        window.set_scene(std::make_shared<research::View>(
            std::shared_ptr<world::World>(world), 
            window.scene(), renderer.output()
        ));
    }


    static const std::string maid_dialogue_key_base = "dialogue_maid_";
    static const size_t maid_dialogue_count = 3;
    static const f64 maid_voice_pitch = 2.3;
    static const f64 maid_voice_speed = 1.8;
    static const f64 maid_player_stop_dist = 3.0;
    static const f64 maid_walk_speed = 2.5;

    struct MaidTarget {
        Vec<3> position;
        std::vector<u8> connections;
    };

    static const std::vector<MaidTarget> maid_targets = {
        // entrance hall
        /* [0] */ (MaidTarget) { { -7.0, 0,  0.0 }, { 1, 2    } }, // center
        /* [1] */ (MaidTarget) { { -6.0, 0, -3.0 }, { 0       } }, // plant
        // hallway
        /* [2] */ (MaidTarget) { {  0.0, 0,  0.0 }, { 0, 3, 6 } }, // center
        // office
        /* [3] */ (MaidTarget) { {  0.0, 0,  6.0 }, { 2, 4, 5 } }, // center
        /* [4] */ (MaidTarget) { { -1.0, 0,  4.0 }, { 3       } }, // plant
        /* [5] */ (MaidTarget) { {  1.5, 0,  4.5 }, { 3       } }, // bookshelf
        // bedroom
        /* [6] */ (MaidTarget) { {  5.0, 0,  0.0 }, { 2, 7, 8 } }, // center
        /* [7] */ (MaidTarget) { {  5.0, 0, -2.5 }, { 6       } }, // bed
        /* [8] */ (MaidTarget) { {  5.0, 0,  2.5 }, { 6       } }, // heater
    };
    static const u64 maid_start_target = 2;

    static std::pair<Character, Interior::CharacterUpdate> create_maid(
        Scene& scene
    ) {
        auto rng = std::make_shared<StatefulRNG>();
        auto dialogue_origin = std::make_shared<Vec<3>>();
        std::shared_ptr<Interactable> interactable = scene.interactables.create(
            [scene = &scene, rng, dialogue_origin]() {
                std::string dialogue_key = maid_dialogue_key_base
                    + std::to_string(rng->next_u64() % maid_dialogue_count);
                const engine::Localization::LoadArgs local_ref = scene->world
                    ->settings.localization();
                const engine::Localization& local = scene
                    ->get<engine::Localization>(local_ref);
                scene->dialogues.say(Dialogue(
                    std::string(local.text("dialogue_maid_name")), 
                    std::string(local.text(dialogue_key)),
                    &voice::voiced,
                    maid_voice_pitch, maid_voice_speed,
                    *dialogue_origin, maid_player_stop_dist
                ));
            }
        );
        static const u64 no_action = UINT64_MAX;
        Character character = Character(
            &human::female, &human::maid,
            { 0, 0, 0 }, no_action
        );
        auto current_target = std::make_shared<u64>(maid_start_target);
        return {
            std::move(character),
            [
                rng = std::move(rng),
                interactable = std::move(interactable), 
                dialogue_origin = std::move(dialogue_origin),
                current_target = std::move(current_target)
            ](
                Character& self, Scene& scene, const engine::Window& window
            ) {
                (void) window;
                interactable->pos = self.position + Vec<3>(0.0, 1.0, 0.0);
                *dialogue_origin = self.position;
                f64 player_dist
                    = (scene.player.character.position - self.position).len();
                bool player_is_close = player_dist <= maid_player_stop_dist;
                bool is_walking = self.action.animation_id 
                    == (u64) human::Animation::Walk;
                // if player is close stop walking
                if(player_is_close && is_walking) {
                    self.action.animation_id = no_action;
                }
                // return if the current action is still going
                if(self.action.animation_id != no_action) { return; }
                // choose a new action
                if(player_is_close) {
                    self.face_towards(scene.player.character.position);
                }
                if(player_is_close || rng->next_bool()) {
                    f64 duration = 1.0 + rng->next_f64() * 2.5;
                    self.action = Character::Action(
                        (u64) human::Animation::Stand, duration
                    );
                    return;
                }
                const MaidTarget& old_target = maid_targets[*current_target];
                f64 old_target_d = (old_target.position - self.position).len();
                bool at_old_target = old_target_d <= 0.1;
                Vec<3> dest;
                if(!at_old_target) {
                    dest = old_target.position;
                } else {
                    u64 next_target_conn_i = (u64) (
                        rng->next_f64() * (f64) old_target.connections.size()
                    );
                    *current_target = old_target.connections[next_target_conn_i];
                    dest = maid_targets[*current_target].position;
                }
                f64 dist = (self.position - dest).len();
                f64 duration = dist / maid_walk_speed;
                self.action = Character::Action(
                    (u64) human::Animation::Walk, dest, duration
                );
                self.face_towards(dest);
            }
        };
    }


    Interior mansion = {
        (engine::Model::LoadArgs) {
            "res/interiors/mansion.glb", Renderer::model_attribs,
            engine::FaceCulling::Enabled
        },
        {
            (Interior::Room) {
                "entrance",
                RelCollider({ -11, -1, -5 }, { 7, 6, 10 }),
                {
                    // wall colliders
                    RelCollider({  -4, -1, -5.5 }, { 1, 6,  4 }),
                    RelCollider({ -11, -1, -5.5 }, { 8, 6,  1 }),
                    RelCollider({ -11, -1, -5.5 }, { 1, 6, 11 }),
                    RelCollider({ -11, -1,  4.5 }, { 8, 6,  1 }),
                    RelCollider({  -4, -1,  1.5 }, { 1, 6,  4 }),
                    // furniture
                    RelCollider({ -5.5, -1, -4 }, { 1, 6, 1 }), // plant
                    RelCollider({ -9.0, -1,  2 }, { 4, 6, 2 })  // sofa
                }
            },
            (Interior::Room) {
                "hallway",
                RelCollider({ -1004, -1, -1000 }, { 1008, 6, 2000 }),
                {
                    // wall colliders
                    RelCollider({ -4.00, -1, -2.5 }, { 8.00, 6, 1.0 }),
                    RelCollider({ -4.00, -1, -1.5 }, { 0.25, 6, 0.5 }),
                    RelCollider({ -4.00, -1,  1.0 }, { 0.25, 6, 0.5 }),
                    RelCollider({ -4.00, -1,  1.5 }, { 3.00, 6, 1.0 }),
                    RelCollider({  1.00, -1,  1.5 }, { 3.00, 6, 1.0 }),
                    RelCollider({  3.75, -1, -1.5 }, { 0.25, 6, 0.5 }),
                    RelCollider({  3.75, -1,  1.0 }, { 0.25, 6, 0.5 })
                }
            },
            (Interior::Room) {
                "bedroom",
                RelCollider({ -1000, -1, -1000 }, { 2000, 6, 2000 }),
                {
                    // wall colliders
                    RelCollider({  3, -1, -5.5 }, { 1, 6,  4 }),
                    RelCollider({  3, -1,  1.5 }, { 1, 6,  4 }),
                    RelCollider({  3, -1,  4.5 }, { 8, 6,  1 }),
                    RelCollider({  3, -1, -5.5 }, { 8, 6,  1 }),
                    RelCollider({ 10, -1, -5.5 }, { 1, 6, 11 }),
                    // furniture
                    RelCollider({ 4.0, -1, -4.5 }, { 2, 6, 1.0 }), // nightstand
                    RelCollider({ 6.0, -1, -4.5 }, { 4, 6, 4.5 }), // bed
                    RelCollider({ 4.5, -1,  3.0 }, { 1, 6, 1.0 }), // heater
                    RelCollider({ 8.5, -1,  3.0 }, { 1, 6, 1.0 })  // plant
                }
            },
            (Interior::Room) {
                "office",
                RelCollider({ -3.5, -1, 2 }, { 7, 6, 10 }),
                {
                    // wall colliders
                    RelCollider({ -4.00, -1,   1.5 }, { 3.00, 6,  1.0 }),
                    RelCollider({ -4.00, -1,   1.5 }, { 1.00, 6, 11.0 }),
                    RelCollider({ -4.00, -1,  11.5 }, { 8.00, 6,  1.0 }),
                    RelCollider({  3.00, -1,   1.5 }, { 1.00, 6, 11.0 }),
                    RelCollider({  1.00, -1,  1.5 }, { 3.00, 6,  1.0 }),
                    // furniture
                    RelCollider({ -2.5, -1,  3 }, { 1, 6, 1 }), // plant
                    RelCollider({  2.0, -1,  3 }, { 1, 6, 3 }), // bookshelf
                    RelCollider({ -2.0, -1,  8 }, { 4, 6, 2 }), // desk
                    RelCollider({ -0.5, -1, 10 }, { 1, 6, 1 })  // chair
                }
            }
        },
        { -7, 0, -3 }, // start position
        { -7, 1.5, -4.5 }, // exit interactable
        {
            // entrance room windows
            DirectionalLight::in_direction_from({ 1, -1, -1 }, { -13, 6.5, 6.5 }, 4.5, 10.0),
            // office windows
            DirectionalLight::in_direction_from({ 1, -1, -1 }, { -6.5, 6.5, 13 }, 4.5, 10.0),
            // bedroom windows
            DirectionalLight::in_direction_from({ -1, -1, -1 }, { 13, 6.5, 6.5 }, 4.5, 10.0),
            // hallway windows
            DirectionalLight::in_direction_to({ 0, -1, 1 }, { 0, 0, 0 }, 3.0, 10.0)
        },
        0.001,
        Vec<3>(-1, 1, 1).normalized() * 15.0,
        Mat<3>::rotate_y(pi / 4),
        {
            (Interior::Interaction) { Vec<3>(0, 1.5, 9), &open_research_view }
        },
        {
            (Interior::CharacterConstructor) &create_maid
        }
    };

}