
#pragma once

#include "character.hpp"

namespace houseofatmos::human {

    enum struct Animation {
        Stand = 0, Walk = 1, Swim = 2, HorseSit = 3, HorseRide = 4,
        DefaultValue = Stand,
        MaximumValue = HorseRide
    };

    static const inline CharacterType character_type = {
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


    static const inline engine::Texture::LoadArgs player = {
        "res/entities/player.png", 
        engine::Texture::vertical_mirror // GLTF stores images flipped
    };    


    static const inline RelCollider collider
        = RelCollider({ -0.25, 0, -0.25 }, { 0.5, 2, 0.5 });


    static void load_resources(engine::Scene& scene) {
        scene.load(engine::Model::Loader(character.model));
        scene.load(engine::Texture::Loader(player));
    }

}