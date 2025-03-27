
#pragma once

#include <engine/audio.hpp>
#include "dialogue.hpp"



namespace houseofatmos::sound {

    using Sound = houseofatmos::engine::Sound;


    static inline const Sound::LoadArgs build 
        = Sound::LoadArgs("res/sounds/build.ogg", 0.05);

    static inline const Sound::LoadArgs demolish 
        = Sound::LoadArgs("res/sounds/demolish.ogg", 0.05);

    static inline const Sound::LoadArgs terrain_mod 
        = Sound::LoadArgs("res/sounds/terrain_mod.ogg", 0.05);

    static inline const Sound::LoadArgs error 
        = Sound::LoadArgs("res/sounds/error.ogg", 0.05);

    static inline const Sound::LoadArgs step 
        = Sound::LoadArgs("res/sounds/step.ogg", 0.025);

    static inline const Sound::LoadArgs swim 
        = Sound::LoadArgs("res/sounds/swim.ogg", 0.1);

    static inline const Sound::LoadArgs horse 
        = Sound::LoadArgs("res/sounds/horse.ogg", 0.1);

    static inline const Sound::LoadArgs chugga 
        = Sound::LoadArgs("res/sounds/chugga.ogg", 0.025);

    static inline const Sound::LoadArgs train_whistle 
        = Sound::LoadArgs("res/sounds/train_whistle.ogg", 0.0);


    inline void load_sounds(engine::Scene& scene) {
        scene.load(build);
        scene.load(demolish);
        scene.load(terrain_mod);
        scene.load(error);
        scene.load(step);
        scene.load(swim);
        scene.load(horse);
        scene.load(chugga);
        scene.load(train_whistle);
    }

}



namespace houseofatmos::voice {

    using Sound = houseofatmos::engine::Sound;
    using Voice = houseofatmos::Dialogue::Voice;


    extern const Voice voiced;
    extern const Voice popped;


    inline void load_voices(engine::Scene& scene) {
        voiced.load(scene);
        popped.load(scene);
    }

}



namespace houseofatmos::audio_const {

    using Soundtrack = houseofatmos::engine::Soundtrack;

    extern const Soundtrack::LoadArgs soundtrack;


    inline void load_all(engine::Scene& scene) {
        sound::load_sounds(scene);
        voice::load_voices(scene);
        scene.load(soundtrack);
    }

}