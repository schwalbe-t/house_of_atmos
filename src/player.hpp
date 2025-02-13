
#pragma once

#include <engine/arena.hpp>
#include <engine/math.hpp>
#include <engine/model.hpp>
#include <engine/audio.hpp>
#include "collider.hpp"
#include "renderer.hpp"
#include "character.hpp"
#include "audio_const.hpp"

namespace houseofatmos {

    using namespace houseofatmos::engine::math;


    struct Player {

        static const inline Character::Type character_type = {
            (engine::Model::LoadArgs) {
                "res/entities/human.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            },
            Vec<3>(0, 0, 1), // the direction the model faces without rotation
            {
                // note: 'x / 24.0' essentially means "'x' Blender frames"
                /* Animation::Idle */ (Character::AnimationImpl) {},
                /* Animation::Stand */ (Character::AnimationImpl) {
                    "idle", 30.0 / 24.0, 0.0
                },
                /* Animation::Walk */ (Character::AnimationImpl) {
                    "walk", (30.0 / 24.0) / 2.0, 0.0,
                    &sound::step, (15.0 / 24.0) / 2.0, (8.0 / 24.0) / 2.0
                },
                /* Animation::Swim */ (Character::AnimationImpl) {
                    "swim", 40.0 / 24.0, 0.0,
                    &sound::swim, 40.0 / 24.0, 10.0 / 24.0
                },
                /* Animation::HorseSit */ (Character::AnimationImpl) {
                    "ride_idle"
                },
                /* Animation::HorseRide */ (Character::AnimationImpl) {
                    "ride", (24.0 / 24.0) / 3.1, -0.05, // simulate inertia
                    &sound::step, (24.0 / 24.0) / 3.1, 0.0
                }
            }
        };

        static const inline engine::Texture::LoadArgs character_variant = {
            "res/entities/player.png", 
            engine::Texture::vertical_mirror // GLTF stores images flipped
        };

        static const inline RelCollider collider
            = RelCollider({ -0.25, 0, -0.25 }, { 0.5, 2, 0.5 });
    
        struct Serialized {
            Vec<3> position;
            f64 angle;
        };


        public:
        Character character = Character(
            &Player::character_type, &Player::character_variant,
            { 0, 0, 0 }
        );
        Vec<3> next_step;
        Vec<3> confirmed_step;
        bool in_water = false;
        bool is_riding = false;

        Player() {}

        Player(const Serialized& serialized) {
            this->character.position = serialized.position;
            this->character.angle = serialized.angle;
        }

        static void load_resources(engine::Scene& scene) {
            scene.load(engine::Model::Loader(Player::character_type.model));
            scene.load(engine::Texture::Loader(Player::character_variant));
        }

        void update(const engine::Window& window);
        Vec<3> next_x() {
            return this->character.position + this->next_step * Vec<3>(1, 0, 0); 
        }
        Vec<3> next_z() { 
            return this->character.position + this->next_step * Vec<3>(0, 0, 1); 
        }
        void proceed_x() { this->confirmed_step.x() += this->next_step.x(); }
        void proceed_z() { this->confirmed_step.z() += this->next_step.z(); }
        void apply_confirmed_step(engine::Scene& scene, const engine::Window& window);
        void render(
            engine::Scene& scene, const engine::Window& window, 
            const Renderer& renderer
        );

        Serialized serialize() const;

    };

}