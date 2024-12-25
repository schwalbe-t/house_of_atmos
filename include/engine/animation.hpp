
#pragma once

#include "math.hpp"
#include <vector>

namespace houseofatmos::engine {

    using namespace math;


    struct Animation {

        template<typename T>
        static T lerp(T start, T end, f64 t) {
            return start + (end - start) * t;
        }

        template<size_t N, typename T>
        static Vec<N, T> slerp(Vec<N, T> start, Vec<N, T> end, f64 t) {
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


        enum Property {
            Translation, Rotation, Scale
        };

        enum Interpolation {
            Step, Linear
        };

        template<size_t N>
        struct KeyFrame {
            f64 timestamp;
            Vec<N> value;
            Interpolation interpolation;
        };

        struct Channel {
            std::vector<KeyFrame<3>> translation;
            std::vector<KeyFrame<4>> rotation;
            std::vector<KeyFrame<3>> scale;
        };

        struct BoneState {
            Vec<3> translation = { 0, 0, 0 };
            Vec<4> rotation = { 0, 0, 0, 1 };
            Vec<3> scale = { 1, 1, 1 };

            Mat<4> as_transform() const;
        };

        struct Bone {
            Mat<4> inverse_bind;
            std::vector<u16> child_indices;
        };

        struct Skeleton {
            std::vector<Bone> bones;
            u16 root_bone_idx;
            Mat<4> root_transform;
        };


        private:
        std::vector<Channel> channels;
        f64 last_timestamp;


        public:
        Animation() = default;
        Animation(std::vector<Channel>&& channels, f64 last_timestamp);

        f64 length() const { return this->last_timestamp; }

        BoneState compute_state(u16 bone_idx, f64 timestamp) const;
        std::vector<Mat<4>> compute_transformations(
            const Skeleton& skeleton, f64 timestamp
        ) const;

    };

}