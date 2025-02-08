
#pragma once

#include <engine/model.hpp>
#include "../renderer.hpp"
#include "../collider.hpp"

namespace houseofatmos::interior {

    struct Interior {

        struct Room {
            std::string name;
            RelCollider trigger;
            std::vector<RelCollider> colliders;
        };

        engine::Model::LoadArgs model;
        std::vector<Room> rooms;
        std::vector<DirectionalLight> lights;
        f64 shadow_bias;
        Vec<3> camera_offset;
        Mat<3> player_velocity_matrix;

    };


    static inline const Interior mansion = {
        (engine::Model::LoadArgs) {
            "res/interiors/mansion.glb", Renderer::model_attribs,
            engine::FaceCulling::Enabled
        },
        {
            (Interior::Room) {
                "entrance",
                RelCollider({ -10.5, -1, -5 }, { 7, 6, 10 }),
                {}
            },
            (Interior::Room) {
                "hallway",
                RelCollider({ -1004.5, -1, -1000 }, { 1009, 6, 2000 }),
                {}
            },
            (Interior::Room) {
                "bedroom",
                RelCollider({ -1000, -1, -1000 }, { 2000, 6, 2000 }),
                {}
            },
            (Interior::Room) {
                "office",
                RelCollider({ -3.5, -1, 2 }, { 7, 6, 10 }),
                {}
            }
        },
        {
            // entrance room windows
            DirectionalLight::in_direction_from({ 1, -1, -1 }, { -13, 6.5, 6.5 }, 4.5, 10.0),
            // office windows
            DirectionalLight::in_direction_from({ 1, -1, -1 }, { -6.5, 6.5, 13 }, 4.5, 10.0)
        },
        0.001,
        Vec<3>(-1, 1, 1).normalized() * 15.0,
        Mat<3>::rotate_y(pi / 4)
    };

}