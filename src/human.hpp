
#pragma once

#include "character.hpp"
#include "audio_const.hpp"
#include "collider.hpp"

namespace houseofatmos::human {

    enum struct Animation {
        Stand = 0, Walk = 1, Swim = 2, HorseSit = 3, HorseRide = 4,
        DefaultValue = Stand,
        MaximumValue = HorseRide
    };

    static const inline CharacterType<5> character = {
        (engine::Model::LoadArgs) {
            "res/entities/human.glb", Renderer::model_attribs,
            engine::FaceCulling::Disabled
        },
        Vec<3>(0, 0, 1), // the direction the model faces without rotation
        {
            // note: the given animation speeds change with movement
            //     - while not moving, the speed doesn't change
            //     - if it's moving, the given speed is used at 1 unit / second
            //       (final_period = period / velocity)
            // note: 'x / 24.0' essentially means "'x' Blender frames"
            /* Animation::Stand */ (CharacterAnimation) {
                "idle", 30.0 / 24.0, 0.0
            },
            /* Animation::Walk */ (CharacterAnimation) {
                "walk", (30.0 / 24.0) / 2.0 * 5.0, 0.0,
                &sound::step, (15.0 / 24.0) / 2.0 * 5.0, (8.0 / 24.0) / 2.0
            },
            /* Animation::Swim */ (CharacterAnimation) {
                "swim", (40.0 / 24.0) * 2.5, 0.0,
                &sound::swim, (40.0 / 24.0) * 2.5, 10.0 / 24.0
            },
            /* Animation::HorseSit */ (CharacterAnimation) {
                "ride_idle"
            },
            /* Animation::HorseRide */ (CharacterAnimation) {
                "ride", (24.0 / 24.0) / 3.1 * 10, -0.05, // simulate inertia
                &sound::step, (24.0 / 24.0) / 3.1 * 10, 0.0
            }
        }
    };


    static const inline CharacterVariant::LoadArgs player = {
        {
            { "human", "res/entities/player.png" }
        }
    };

    static const inline CharacterVariant::LoadArgs peasant_woman = {
        {
            { "human", "res/entities/peasant.png" },
            { "hair", "res/entities/peasant_hair_woman.png" }
        }
    };


    static const inline RelCollider collider
        = RelCollider({ -0.25, 0, -0.25 }, { 0.5, 2, 0.5 });


    static inline void load_resources(engine::Scene& scene) {
        scene.load(engine::Model::Loader(character.model));
        scene.load(CharacterVariant::Loader(player));
        scene.load(CharacterVariant::Loader(peasant_woman));
    }

}