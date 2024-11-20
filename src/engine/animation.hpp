
#pragma once

#include "math.hpp"
#include <vector>

namespace houseofatmos::engine::animation {

    using namespace houseofatmos::engine::math;


    enum Interpolation {
        ITPL_STEP, ITPL_LINEAR
    };

    struct KeyFrame {
        double timestamp;
        Interpolation translation_itpl;
        Vec<3> translation;
        Interpolation rotation_itpl;
        Vec<4> rotation;
        Interpolation scale_itpl;
        Vec<3> scale;
    };

    struct Animation {
        // one list of keyframes for each bone
        std::vector<std::vector<KeyFrame>> keyframes;

        KeyFrame get_frame(size_t bone, size_t timestamp);

        template<typename B>
        void compute_transforms(
            std::vector<B>& bones, size_t timestamp
        ) {
            std::abort(); // TODO!
        }
    };

}