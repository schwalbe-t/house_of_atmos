
#pragma once

#include "character.hpp"
#include "audio_const.hpp"
#include "collider.hpp"

namespace houseofatmos::human {

    enum struct Animation {
        Stand, Walk, SwimIdle, Swim, HorseSit, HorseRide
    };

    static const inline CharacterType character = {
        (engine::Model::LoadArgs) {
            "res/entities/human.glb", Renderer::model_attribs,
            engine::FaceCulling::Disabled
        },
        Vec<3>(0, 0, 1), // the direction the model faces without rotation
        {
            // note: 'x / 24.0' essentially means "'x' Blender frames"
            /* Animation::Stand */ (CharacterAnimation) {
                "idle", 30.0 / 24.0, 0.0
            },
            /* Animation::Walk */ (CharacterAnimation) {
                "walk", (30.0 / 24.0) / 2.0 * 5.0, 0.0,
                &sound::step, (15.0 / 24.0) / 2.0 * 5.0, (8.0 / 24.0) / 2.0
            },
            /* Animation::SwimIdle */ (CharacterAnimation) {
                "swim", 40.0 / 24.0, 0.0,
                &sound::swim, 40.0 / 24.0, 10.0 / 24.0
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


    static const inline CharacterVariant::LoadArgs count = {
        {
            { "human", "res/entities/count.png" }
        }
    };

    static const inline CharacterVariant::LoadArgs peasant_woman = {
        {
            { "human", "res/entities/peasant.png" },
            { "hair", "res/entities/peasant_hair_woman.png" }
        }
    };

    static const inline CharacterVariant::LoadArgs peasant_man = {
        {
            { "human", "res/entities/peasant.png" },
            { "hair", "res/entities/peasant_hair_man.png" }
        }
    };


    static inline void load_resources(engine::Scene& scene) {
        scene.load(engine::Model::Loader(character.model));
        scene.load(CharacterVariant::Loader(count));
        scene.load(CharacterVariant::Loader(peasant_woman));
        scene.load(CharacterVariant::Loader(peasant_man));
    }


    static const inline RelCollider collider
        = RelCollider({ -0.25, 0, -0.25 }, { 0.5, 2, 0.5 });

}