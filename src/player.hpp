
#pragma once

#include <engine/arena.hpp>
#include <engine/math.hpp>
#include <engine/model.hpp>
#include "collider.hpp"
#include "renderer.hpp"

namespace houseofatmos {

    using namespace houseofatmos::engine::math;


    struct Balance {
        u64 coins;

        bool pay_coins(u64 amount) {
            if(amount > this->coins) {
                engine::info("Not enough coins! (" + std::to_string(this->coins)
                    + "/" + std::to_string(amount) + ")"
                );
                return false;
            }
            this->coins -= amount;
            engine::info("Payed " + std::to_string(amount) + " coins "
                "(now " + std::to_string(this->coins) + ")"
            );
            return true;
        }
    };


    struct Player {
    
        struct Serialized {
            Vec<3> position;
            f64 angle;
        };


        static const inline engine::Model::LoadArgs player_model = {
            "res/entities/player.glb", Renderer::model_attribs
        };

        static const inline RelCollider collider
            = RelCollider({ -0.25, 0, -0.25 }, { 0.5, 1, 0.5 });


        private:
        f64 angle;
        std::string anim_name;
        f64 anim_time;
        f64 anim_speed;

        void set_anim_idle();
        void set_anim_walk();
        void set_anim_swim();

        public:
        Vec<3> position;
        Vec<3> next_step;
        bool in_water;

        Player() {
            this->position = { 0, 0, 0 };
            this->angle = 0;
            this->in_water = false;
            this->set_anim_idle();
        }

        Player(const Serialized& serialized, const engine::Arena& buffer) {
            (void) buffer;
            this->position = serialized.position;
            this->angle = serialized.angle;
            this->in_water = false;
            this->set_anim_idle();
        }

        static void load_model(engine::Scene& scene) {
            scene.load(engine::Model::Loader(Player::player_model));
        }

        void update(engine::Window& window);
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