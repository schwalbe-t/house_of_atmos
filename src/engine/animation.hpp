
#pragma once

#include "math.hpp"
#include <vector>
#include <cassert>

namespace houseofatmos::engine::animation {

    using namespace houseofatmos::engine::math;


    enum Interpolation {
        MISSING, STEP, LINEAR
    };


    struct KeyFrame {
        float timestamp = 0.0;
        Interpolation translation_itpl = Interpolation::MISSING;
        Vec<3> translation = Vec<3>(0.0, 0.0, 0.0);
        Interpolation rotation_itpl = Interpolation::MISSING;
        Vec<4> rotation = Vec<4>(0.0, 0.0, 0.0, 1.0);
        Interpolation scale_itpl = Interpolation::MISSING;
        Vec<3> scale = Vec<3>(1.0, 1.0, 1.0);
    };


    static Mat<4> build_anim_transform(
        const Vec<3>& translation, const Vec<4>& rotation, const Vec<3>& scale
    ) {
        return Mat<4>::translate(translation)
            * Mat<4>::quaternion(rotation)
            * Mat<4>::scale(scale);
    }

    template<typename B>
    static void propagate_anim_transform(
        const Mat<4>* parent_transform, B& bone, std::vector<B>& bones
    ) {
        if(parent_transform != NULL) {
            bone.anim_transform = bone.anim_transform * *parent_transform;
        }
        for(size_t child_i = 0; child_i < bone.children.size(); child_i += 1) {
            B& child = bones[bone.children[child_i]];
            propagate_anim_transform(&bone.anim_transform, child, bones);
        }
    }

    struct Animation {
        // one list of keyframes for each bone
        std::vector<std::vector<KeyFrame>> keyframes;
        double length;

        void complete_keyframe_values();
        void compute_length();

        KeyFrame compute_frame(size_t bone, double timestamp) const;

        template<typename B>
        void compute_transforms(
            std::vector<B>& bones, size_t timestamp
        ) const {
            size_t joint_count = this->keyframes.size();
            assert(bones.size() == joint_count);
            for(size_t joint_i = 0; joint_i < joint_count; joint_i += 1) {
                KeyFrame frame = this->compute_frame(joint_i, timestamp);
                B& bone = bones[joint_i];
                bone.anim_transform = build_anim_transform(
                    frame.translation, frame.rotation, frame.scale
                );
            }
            for(size_t joint_i = 0; joint_i < joint_count; joint_i += 1) {
                B& bone = bones[joint_i];
                if(bone.has_parent) { continue; }
                propagate_anim_transform(NULL, bone, bones);
            }
            for(size_t joint_i = 0; joint_i < joint_count; joint_i += 1) {
                B& bone = bones[joint_i];
                bone.anim_transform = bone.anim_transform * bone.inverse_bind;
            }
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