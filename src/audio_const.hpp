
#pragma once

#include <engine/audio.hpp>



namespace houseofatmos::sound {

    using Sound = houseofatmos::engine::Sound;


    static inline const Sound::LoadArgs build = (Sound::LoadArgs) {
        "res/sounds/build.ogg", 0.9, 0.05
    };

    static inline const Sound::LoadArgs terrain_mod = (Sound::LoadArgs) {
        "res/sounds/terrain_mod.ogg", 0.9, 0.05
    };

    static inline const Sound::LoadArgs error = (Sound::LoadArgs) {
        "res/sounds/error.ogg", 0.9, 0.05
    };


    inline void load_sounds(engine::Scene& scene) {
        scene.load(Sound::Loader(build));
        scene.load(Sound::Loader(terrain_mod));
        scene.load(Sound::Loader(error));
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