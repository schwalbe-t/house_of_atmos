
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
        Vec<3> player_start_pos;
        Vec<3> exit_interactable;
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
                RelCollider({ -11, -1, -5 }, { 7, 6, 10 }),
                {
                    // wall colliders
                    RelCollider({  -4, -1, -5.5 }, { 1, 6,  4 }),
                    RelCollider({ -11, -1, -5.5 }, { 8, 6,  1 }),
                    RelCollider({ -11, -1, -5.5 }, { 1, 6, 11 }),
                    RelCollider({ -11, -1,  4.5 }, { 8, 6,  1 }),
                    RelCollider({  -4, -1,  1.5 }, { 1, 6,  4 })
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
                    RelCollider({ 10, -1, -5.5 }, { 1, 6, 11 })
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
                    RelCollider({  1.00, -1,  1.5 }, { 3.00, 6,  1.0 })
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
        Mat<3>::rotate_y(pi / 4)
    };

}