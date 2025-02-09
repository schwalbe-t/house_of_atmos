
#include <engine/rng.hpp>
#include "sound.hpp"

namespace houseofatmos {

    using namespace houseofatmos::engine::math;


    Sound Sound::from_resource(const Sound::LoadArgs& args) {
        auto audio = engine::Audio::from_resource({ args.path });
        return Sound(std::move(audio), args.base_pitch, args.pitch_variation);
    }

    static StatefulRNG pitch_rng;

    void Sound::randomize_pitch() {
        f64 variation = pitch_rng.next_f64() * 2.0 - 1.0;
        f64 pitch = this->base_pitch() + this->pitch_variation() * variation;
        this->player.set_pitch(pitch);
    }

}