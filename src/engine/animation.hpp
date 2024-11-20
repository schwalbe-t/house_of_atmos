
#pragma once

#include "math.hpp"
#include <vector>

namespace houseofatmos::engine::animation {

    using namespace houseofatmos::engine::math;


    enum Interpolation {
        MISSING, STEP, LINEAR
    };

    struct KeyFrame {
        float timestamp;
        Interpolation translation_itpl = Interpolation::MISSING;
        Vec<3> translation;
        Interpolation rotation_itpl = Interpolation::MISSING;
        Vec<4> rotation;
        Interpolation scale_itpl = Interpolation::MISSING;
        Vec<3> scale;
    };

    struct Animation {
        // one list of keyframes for each bone
        std::vector<std::vector<KeyFrame>> keyframes;
        double length;

        void complete_keyframe_values();
        void compute_length();

        KeyFrame compute_frame(size_t bone, double timestamp);

        template<typename B>
        void compute_transforms(
            std::vector<B>& bones, size_t timestamp
        ) {
            std::abort(); // TODO!
        }
    };


    template<typename T>
    T lerp(T start, T end, double t) {
        return start + (end - start) * t;
    }

    template<int N>
    Vec<N> slerp(Vec<N> start, Vec<N> end, double t) {
        double dot = start.dot(end);
        if(dot < 0) {
            end *= -1;
            dot *= -1;
        }
        if(dot > 0.9995) { return lerp(start, end, t); }
        double theta = acos(dot);
        double sin_theta = sin(theta);
        return start * sin((1 - t) * theta) / sin_theta
            + end * sin(t * theta) / sin_theta;
    }

    template<typename T>
    T interpolate(T start, T end, double t, Interpolation i) {
        if(i != Interpolation::LINEAR) { return start; }
        return lerp(start, end, t);
    }

    template<int N>
    Vec<N> interpolate_spherical(
        Vec<N> start, Vec<N> end, double t, Interpolation i
    ) {
        if(i != Interpolation::LINEAR) { return start; }
        return slerp(start, end, t);
    }

}