
#pragma once

#include <engine/math.hpp>
#include <engine/model.hpp>
#include "renderer.hpp"

namespace houseofatmos {

    using namespace houseofatmos::engine::math;


    struct Player {
    
        static const inline engine::Model::LoadArgs player_model = {
            "res/player.gltf", Renderer::model_attribs
        };


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
        bool in_water;

        Player() {
            this->position = { 0, 0, 0 };
            this->angle = 0;
            this->in_water = false;
            this->set_anim_idle();
        }

        static void load_model(engine::Scene& scene) {
            scene.load(engine::Model::Loader(Player::player_model));
        }

        void update(const engine::Window& window);
        void render(engine::Scene& scene, const Renderer& renderer);

    };

}