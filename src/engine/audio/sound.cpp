
#include <engine/audio.hpp>
#include <engine/rng.hpp>

namespace houseofatmos::engine {

    using namespace houseofatmos::engine::math;


    Sound Sound::from_resource(const Sound::LoadArgs& args) {
        auto audio = engine::Audio::from_resource({ args.path });
        return Sound(std::move(audio), args.base_pitch, args.pitch_var);
    }

    static StatefulRNG pitch_rng;

    f64 Sound::generate_pitch() const {
        f64 factor = pitch_rng.next_f64() * 2.0 - 1.0;
        return this->base_pitch + this->pitch_variation * factor;
    }

}