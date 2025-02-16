
#pragma once

#include <engine/audio.hpp>



namespace houseofatmos::sound {

    using Sound = houseofatmos::engine::Sound;


    static inline const Sound::LoadArgs build = (Sound::LoadArgs) {
        "res/sounds/build.ogg", 1.0, 0.05
    };

    static inline const Sound::LoadArgs demolish = (Sound::LoadArgs) {
        "res/sounds/demolish.ogg", 1.0, 0.05
    };

    static inline const Sound::LoadArgs terrain_mod = (Sound::LoadArgs) {
        "res/sounds/terrain_mod.ogg", 1.0, 0.05
    };

    static inline const Sound::LoadArgs error = (Sound::LoadArgs) {
        "res/sounds/error.ogg", 1.0, 0.05
    };


    static inline const Sound::LoadArgs step = (Sound::LoadArgs) {
        "res/sounds/step.ogg", 1.0, 0.025
    };

    static inline const Sound::LoadArgs swim = (Sound::LoadArgs) {
        "res/sounds/swim.ogg", 1.0, 0.1
    };


    static inline const Sound::LoadArgs horse = (Sound::LoadArgs) {
        "res/sounds/horse.ogg", 1.0, 0.1
    };


    inline void load_sounds(engine::Scene& scene) {
        scene.load(Sound::Loader(build));
        scene.load(Sound::Loader(demolish));
        scene.load(Sound::Loader(terrain_mod));
        scene.load(Sound::Loader(error));
        scene.load(Sound::Loader(step));
        scene.load(Sound::Loader(swim));
        scene.load(Sound::Loader(horse));
    }

    inline void set_gain(engine::Scene& scene, f64 value) {
        scene.get<engine::Sound>(build).set_gain(value);
        scene.get<engine::Sound>(demolish).set_gain(value);
        scene.get<engine::Sound>(terrain_mod).set_gain(value);
        scene.get<engine::Sound>(error).set_gain(value);
        scene.get<engine::Sound>(step).set_gain(value);
        scene.get<engine::Sound>(swim).set_gain(value);
        scene.get<engine::Sound>(horse).set_gain(value);
    }

}



namespace houseofatmos::audio_const {

    using Soundtrack = houseofatmos::engine::Soundtrack;

    static inline const Soundtrack::LoadArgs soundtrack = (Soundtrack::LoadArgs) {
        {
            "res/soundtrack/track_1.ogg",
            "res/soundtrack/track_2.ogg"
        },
        Soundtrack::no_repetition
    };


    inline void load_all(engine::Scene& scene) {
        sound::load_sounds(scene);
        scene.load(Soundtrack::Loader(soundtrack));
    }

}