
#pragma once

#include "particles.hpp"

namespace houseofatmos::particle {

    static inline Vec<3> wind_direction(const engine::Window& window) {
        f64 angle = sin(window.time() / 100) * sin(window.time() / 230) * pi;
        f64 speed = 1.0 + sin(window.time() / 60) * 0.5;
        return Vec<3>(cos(angle), 0, sin(angle)) * speed;
    }


    static inline Vec<3> smoke_position(
        const Particle& particle, const engine::Window& window
    ) {
        return particle.start_pos 
            + Vec<3>(0, 1, 0) * particle.age
            + wind_direction(window) * particle.age;
    }

    static inline Vec<2> smoke_size(
        const Particle& particle, const engine::Window& window
    ) {
        (void) window;
        return Vec<2>(1, 1) + Vec<2>(4.0/10.0, 4.0/10.0) * particle.age;
    }

    static inline const Particle::Type smoke_large = Particle::Type(
        engine::Texture::LoadArgs("res/particles.png"),
        Vec<2>(0, 0), // position on texture
        Vec<2>(32, 32), // size on texture
        10.0, // duration in seconds
        smoke_position,
        smoke_size
    );

    static inline const Particle::Type smoke_medium = Particle::Type(
        engine::Texture::LoadArgs("res/particles.png"),
        Vec<2>(32, 0), // position on texture
        Vec<2>(32, 32), // size on texture
        10.0, // duration in seconds
        smoke_position,
        smoke_size
    );

    static inline const Particle::Type smoke_small = Particle::Type(
        engine::Texture::LoadArgs("res/particles.png"),
        Vec<2>(64, 0), // position on texture
        Vec<2>(32, 32), // size on texture
        10.0, // duration in seconds
        smoke_position,
        smoke_size
    );

    inline const Particle::Type* random_smoke(StatefulRNG& rng) {
        u64 size = (u64) (rng.next_f64() * 3.0);
        return size == 0? &smoke_small
            : size == 1? &smoke_medium
            : &smoke_large;
    }


    static inline Vec<3> tree_shedding_position(
        const Particle& particle, const engine::Window& window
    ) {
        return particle.start_pos 
            + Vec<3>(0, -1, 0) * particle.age
            + Vec<3>(0.25, 0, 0) * sin(particle.age / 2.5 * 2*pi)
            + wind_direction(window) * particle.age;
    }

    static inline Vec<2> tree_shedding_size(
        const Particle& particle, const engine::Window& window
    ) {
        (void) particle;
        (void) window;
        return Vec<2>(0.5, 0.5);
    }

    static inline const Particle::Type falling_leaf = Particle::Type(
        engine::Texture::LoadArgs("res/particles.png"),
        Vec<2>(0, 40), // position on texture
        Vec<2>(8, 8), // size on texture
        10.0, // duration in seconds
        tree_shedding_position,
        tree_shedding_size
    );

    static inline const Particle::Type falling_needle = Particle::Type(
        engine::Texture::LoadArgs("res/particles.png"),
        Vec<2>(16, 40), // position on texture
        Vec<2>(8, 8), // size on texture
        10.0, // duration in seconds
        tree_shedding_position,
        tree_shedding_size
    );


    inline void load_textures(engine::Scene& scene) {
        scene.load(smoke_large.texture);
        // all other particles also use 'res/particles.png'
    }

}