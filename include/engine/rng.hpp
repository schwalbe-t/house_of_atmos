
#pragma once

#include "math.hpp"

namespace houseofatmos::engine::math {

    u64 random_init();


    double perlin_noise(u32 seed, const Vec<2>& pos);


    struct StatefulRNG {
        u64 state;

        StatefulRNG(u64 seed = random_init());

        f64 next_f64();
        u64 next_u64();
        i64 next_i64();
        bool next_bool() { return this->next_f64() < 0.5; }
    };

}