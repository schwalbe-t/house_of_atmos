
#include <engine/animation.hpp>
#include <engine/logging.hpp>
#include <cassert>

namespace houseofatmos::engine {

    Mat<4> Animation::BoneState::as_transform() const {
        return Mat<4>::translate(this->translation)
            * Mat<4>::quaternion_xyzw(this->rotation)
            * Mat<4>::scale(this->scale);
    }


    Animation::Animation(std::vector<Channel>&& channels) {
        this->channels = std::move(channels);
    }

    template<size_t N>
    static size_t last_keyframe_idx(
        const std::vector<Animation::KeyFrame<N>>& keyframes, f64 timestamp
    ) {
        assert(keyframes.size() != 0);
        for(size_t kf_i = 0; kf_i < keyframes.size(); kf_i += 1) {
            const Animation::KeyFrame<N>& keyframe = keyframes[kf_i];
            if(keyframe.timestamp <= timestamp) { continue; }
            // this is the next keyframe
            if(kf_i == 0) {
                // no previous keyframe - this is the first one
                return kf_i;
            }
            // return the previous keyframe 
            return kf_i - 1;
        }
        // no next keyframe found - last keyframe was the last one
        return keyframes.size() - 1;
    }

    template<size_t N>
    static Vec<N> compute_property_value(
        const std::vector<Animation::KeyFrame<N>>& keyframes,
        bool spherical,
        f64 timestamp
    ) {
        size_t last_kf_i = last_keyframe_idx(keyframes, timestamp);
        size_t next_kf_i = std::min(last_kf_i + 1, keyframes.size() - 1);
        const Animation::KeyFrame<N>& last_kf = keyframes[last_kf_i];
        const Animation::KeyFrame<N>& next_kf = keyframes[next_kf_i];
        f64 time_diff = next_kf.timestamp - last_kf.timestamp;
        f64 t = (timestamp - last_kf.timestamp) / time_diff;
        switch(next_kf.interpolation) {
            case Animation::Step: return last_kf.value;
            case Animation::Linear: return spherical
                ? Animation::slerp(last_kf.value, next_kf.value, t)
                : Animation::lerp(last_kf.value, next_kf.value, t);
        }
        error("Unhandled interpolation type in 'compute_property_value'");
        return Vec<N>();
    }

    Animation::BoneState Animation::compute_state(u16 bone_idx, f64 timestamp) const {
        const Channel& channel = this->channels[bone_idx];
        return {
            compute_property_value<3>(channel.translation, false, timestamp),
            compute_property_value<4>(channel.rotation, true, timestamp),
            compute_property_value<3>(channel.scale, false, timestamp)
        };
    }

    static void propagate_transforms(
        const Animation::Skeleton& skeleton,
        const Mat<4>& parent_transform, u16 bone_idx,
        std::vector<Mat<4>>& transforms
    ) {
        Mat<4>& transform = transforms[bone_idx];
        transform = transform * parent_transform;
        const Animation::Bone& bone = skeleton.bones[bone_idx];
        for(size_t child_i = 0; child_i < bone.child_indices.size(); child_i += 1) {
            u16 child_bone_idx = bone.child_indices[child_i];
            propagate_transforms(skeleton, transform, child_bone_idx, transforms);
        }
    }

    std::vector<Mat<4>> Animation::compute_transforms(
        const Skeleton& skeleton, f64 timestamp
    ) const {
        assert(skeleton.bones.size() == this->channels.size());
        auto result = std::vector<Mat<4>>(skeleton.bones.size());
        for(size_t bone_idx = 0; bone_idx < skeleton.bones.size(); bone_idx += 1) {
            BoneState state = this->compute_state(bone_idx, timestamp);
            result[bone_idx] = state.as_transform();
        }
        propagate_transforms(
            skeleton, skeleton.root_transform, skeleton.root_bone_idx, result
        );
        for(size_t bone_idx = 0; bone_idx < skeleton.bones.size(); bone_idx += 1) {
            const Mat<4>& inv_bind = skeleton.bones[bone_idx].inverse_bind;
            result[bone_idx] = result[bone_idx] * inv_bind;
        }
        return result;
    }

}