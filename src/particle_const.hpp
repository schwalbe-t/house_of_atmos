
#pragma once

#include "particles.hpp"

namespace houseofatmos::particle {

    static inline Vec<3> smoke_position(const Particle& particle) {
        return particle.start_pos + Vec<3>(0, 1, 0) * particle.age;
    }

    static inline Vec<2> smoke_size(const Particle& particle) {
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


    inline void load_textures(engine::Scene& scene) {
        scene.load(smoke_large.texture);
        // all other particles also use 'res/particles.png'
    }

}