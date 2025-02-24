
#pragma once

#include "character.hpp"
#include "audio_const.hpp"
#include "collider.hpp"

namespace houseofatmos::human {

    enum struct Animation {
        Stand, Walk, SwimIdle, Swim, HorseSit, HorseRide, Sit
    };

    // note: 'x / 24.0' essentially means "'x' Blender frames"
    static const inline CharacterAnimation stand = { 
        "idle", 30.0 / 24.0, 0.0 
    };
    static const inline CharacterAnimation walk = {
        "walk", (30.0 / 24.0) / 2.0 * 5.0, 0.0,
        &sound::step, (15.0 / 24.0) / 2.0 * 5.0, (8.0 / 24.0) / 2.0
    };
    static const inline CharacterAnimation swim_idle = {
        "swim", 40.0 / 24.0, 0.0,
        &sound::swim, 40.0 / 24.0, 10.0 / 24.0
    };
    static const inline CharacterAnimation swim = {
        "swim", (40.0 / 24.0) * 2.5, 0.0,
        &sound::swim, (40.0 / 24.0) * 2.5, 10.0 / 24.0
    };
    static const inline CharacterAnimation horse_sit = { 
        "ride_idle" 
    };
    static const inline CharacterAnimation horse_ride = {
        "ride", (24.0 / 24.0) / 3.1 * 10, -0.05, // simulate inertia
        &sound::step, (24.0 / 24.0) / 3.1 * 10, 0.0
    };
    static const inline CharacterAnimation sit_male = { "sit_male" };
    static const inline CharacterAnimation sit_female = { "sit_female" };

    static const inline CharacterType male = {
        (engine::Model::LoadArgs) {
            "res/entities/human.glb", Renderer::model_attribs,
            engine::FaceCulling::Disabled
        },
        Mat<4>(),
        Vec<3>(0, 0, 1), // the direction the model faces without rotation
        {
            // in the order defined by 'human::Animation'
            human::stand, human::walk, 
            human::swim_idle, human::swim,
            human::horse_sit, human::horse_ride, 
            human::sit_male
        }
    };

    static const inline CharacterType female = {
        (engine::Model::LoadArgs) {
            "res/entities/human.glb", Renderer::model_attribs,
            engine::FaceCulling::Disabled
        },
        Mat<4>(),
        Vec<3>(0, 0, 1), // the direction the model faces without rotation
        {
            // in the order defined by 'human::Animation'
            human::stand, human::walk, 
            human::swim_idle, human::swim,
            human::horse_sit, human::horse_ride, 
            human::sit_female
        }
    };

    static const inline CharacterType toddler = {
        (engine::Model::LoadArgs) {
            "res/entities/human.glb", Renderer::model_attribs,
            engine::FaceCulling::Disabled
        },
        Mat<4>::scale(Vec<3>(0.5, 0.5, 0.5)), // model transform
        Vec<3>(0, 0, 1), // the direction the model faces without rotation
        {
            // in the order defined by 'human::Animation'
            human::stand, human::walk, 
            human::swim_idle, human::swim,
            human::horse_sit, human::horse_ride, 
            human::sit_male
        }
    };


    static const inline CharacterVariant::LoadArgs count = {
        {
            { "human", "res/entities/count.png" }
        }
    };

    static const inline CharacterVariant::LoadArgs father = {
        {
            { "human", "res/entities/father.png" }
        }
    };

    static const inline CharacterVariant::LoadArgs peasant_woman = {
        {
            { "human", "res/entities/peasant.png" },
            { "hair", "res/entities/peasant_hair_woman.png" },
            { "dress", "res/entities/peasant_dress_woman.png" }
        }
    };

    static const inline CharacterVariant::LoadArgs peasant_man = {
        {
            { "human", "res/entities/peasant.png" },
            { "hair", "res/entities/peasant_hair_man.png" }
        }
    };

    static const inline CharacterVariant::LoadArgs maid = {
        {
            { "human", "res/entities/maid.png" },
            { "hair", "res/entities/maid_hair.png" },
            { "dress", "res/entities/maid_dress.png" }
        }
    };


    static inline void load_resources(engine::Scene& scene) {
        scene.load(engine::Model::Loader(male.model));
        scene.load(engine::Model::Loader(female.model));
        scene.load(engine::Model::Loader(toddler.model));
        scene.load(CharacterVariant::Loader(count));
        scene.load(CharacterVariant::Loader(father));
        scene.load(CharacterVariant::Loader(peasant_woman));
        scene.load(CharacterVariant::Loader(peasant_man));
        scene.load(CharacterVariant::Loader(maid));
    }


    static const inline RelCollider collider
        = RelCollider({ -0.25, 0, -0.25 }, { 0.5, 2, 0.5 });

}