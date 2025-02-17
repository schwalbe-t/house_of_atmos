
#pragma once

#include <engine/model.hpp>
#include "../renderer.hpp"
#include "../collider.hpp"
#include <functional>

namespace houseofatmos::world {
    struct World;
}

namespace houseofatmos::interior {

    struct Interior {

        struct Room {
            std::string name;
            RelCollider trigger;
            std::vector<RelCollider> colliders;
        };

        struct Interaction {
            Vec<3> position;
            void (*handler)(
                const std::shared_ptr<world::World>& world, 
                const Renderer& renderer, engine::Window& window
            );
        };

        engine::Model::LoadArgs model;
        std::vector<Room> rooms;
        Vec<3> player_start_pos;
        Vec<3> exit_interactable;
        std::vector<DirectionalLight> lights;
        f64 shadow_bias;
        Vec<3> camera_offset;
        Mat<3> player_velocity_matrix;
        std::vector<Interaction> interactions;

    };


    extern Interior mansion;

}