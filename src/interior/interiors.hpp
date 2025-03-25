
#pragma once

#include <engine/model.hpp>
#include <engine/localization.hpp>
#include "../renderer.hpp"
#include "../collider.hpp"
#include "../character.hpp"
#include "../interactable.hpp"
#include <functional>

namespace houseofatmos::world {
    struct World;
}

namespace houseofatmos::interior {

    struct Scene;

    struct Interior {

        struct Room {
            std::string_view name;
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

        using CharacterUpdate = std::function<void (
            Character&, Scene&, const engine::Window& window
        )>;
        using CharacterConstructor = std::pair<Character, CharacterUpdate> (*)(
            Scene&, const Settings&
        );

        engine::Model::LoadArgs model;
        std::vector<Room> rooms;
        Vec<3> player_start_pos;
        Vec<3> exit_interactable;
        std::vector<DirectionalLight> lights;
        f64 shadow_depth_bias;
        f64 shadow_normal_offset;
        bool shadow_out_of_bounds_lit;
        Vec<3> camera_offset;
        Mat<3> player_velocity_matrix;
        std::vector<Interaction> interactions;
        std::vector<CharacterConstructor> characters;

    };


    extern Interior mansion;

}