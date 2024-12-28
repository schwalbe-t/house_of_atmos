
#include <engine/rng.hpp>
#include <Reputeless/PerlinNoise.hpp>
#include <ctime>

namespace houseofatmos::engine::math {

    u64 random_init() {
        return std::time(nullptr);
    }


    f64 perlin_noise(u32 seed, const Vec<2>& pos) {
        return siv::PerlinNoise(seed).octave2D_11(pos.x(), pos.y(), 4);
    }


    static const u64 lcg_mltpl = 6364136223846793005;
    static const u64 lcg_incr = 1442695040888963407;

    u64 random_u64(u64 input) {
        return input * lcg_mltpl + lcg_incr;
    }

    f64 random_f64(u64 input) {
        return fabs((double) random_u64(input)) / (double) UINT64_MAX;
    }




    StatefulRNG::StatefulRNG(u64 seed) {
        this->state = seed;
    }

    u64 StatefulRNG::next_u64() {
        this->state = random_u64(this->state);
        return this->state;
    }

    i64 StatefulRNG::next_i64() {
        const u64 r = this->next_u64();
        return *((const i64*) &r);
    }

    f64 StatefulRNG::next_f64() {
        return (double) this->next_u64() / (double) UINT64_MAX;
    }
    
}