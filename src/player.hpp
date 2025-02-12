
#pragma once

#include <engine/arena.hpp>
#include <engine/math.hpp>
#include <engine/model.hpp>
#include <engine/audio.hpp>
#include "collider.hpp"
#include "renderer.hpp"
#include "toasts.hpp"

namespace houseofatmos {

    using namespace houseofatmos::engine::math;


    struct Balance {

        u64 coins;

        bool pay_coins(u64 amount, Toasts& toasts) {
            if(amount > this->coins) {
                toasts.add_error(
                    "toast_too_expensive", { std::to_string(amount) }
                );
                return false; 
            }
            this->coins -= amount;
            toasts.add_toast(
                "toast_removed_coins", { std::to_string(amount) }
            );
            return true;
        }

        void add_coins(u64 amount, Toasts& toasts) {
            this->coins += amount;
            toasts.add_toast(
                "toast_added_coins", { std::to_string(amount) }
            );
        }
    };


    struct Player {
    
        struct Serialized {
            Vec<3> position;
            f64 angle;
        };


        static const inline engine::Model::LoadArgs player_model = {
            "res/entities/player.glb", Renderer::model_attribs,
            engine::FaceCulling::Disabled
        };

        static const inline RelCollider collider
            = RelCollider({ -0.25, 0, -0.25 }, { 0.5, 2, 0.5 });


        private:
        f64 angle;
        std::string anim_name;
        f64 anim_time;
        f64 anim_speed;
        const engine::Sound::LoadArgs* sound;
        f64 sound_time;
        f64 sound_speed;

        void set_anim_idle();
        void set_anim_walk();
        void set_anim_swim();
        void set_anim_ride_idle();
        void set_anim_ride();

        public:
        Vec<3> position;
        Vec<3> next_step;
        bool in_water;
        bool is_riding;

        Player() {
            this->position = { 0, 0, 0 };
            this->angle = 0;
            this->in_water = false;
            this->is_riding = false;
            this->set_anim_idle();
        }

        Player(const Serialized& serialized, const engine::Arena& buffer) {
            (void) buffer;
            this->position = serialized.position;
            this->angle = serialized.angle;
            this->in_water = false;
            this->is_riding = false;
            this->set_anim_idle();
        }

        static void load_model(engine::Scene& scene) {
            scene.load(engine::Model::Loader(Player::player_model));
        }

        f64 current_angle() const { return this->angle; }

        void update(engine::Scene& scene, engine::Window& window);
        Vec<3> next_x() {
            return this->position + this->next_step * Vec<3>(1, 0, 0); 
        }
        Vec<3> next_z() { 
            return this->position + this->next_step * Vec<3>(0, 0, 1); 
        }
        void proceed_x() { this->position.x() += this->next_step.x(); }
        void proceed_z() { this->position.z() += this->next_step.z(); }
        void render(engine::Scene& scene, const Renderer& renderer);

        Serialized serialize(engine::Arena& buffer) const;

    };

}