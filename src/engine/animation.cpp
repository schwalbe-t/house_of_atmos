
#include "animation.hpp"

namespace houseofatmos::engine::animation {

    static void complete_keyframe_gaps(
        std::vector<KeyFrame>& keyframes,
        Vec<3>& last_transl, size_t& last_transl_i,
        Vec<4>& last_rot, size_t& last_rot_i,
        Vec<3>& last_scale, size_t& last_scale_i
    ) {
        for(size_t kf_i = 1; kf_i < keyframes.size(); kf_i += 1) {
            KeyFrame& kf = keyframes[kf_i];
            bool insert_translation
                = kf.translation_itpl != Interpolation::MISSING
                && kf_i > last_transl_i + 1;
            bool insert_rotation
                = kf.rotation_itpl != Interpolation::MISSING
                && kf_i > last_rot_i + 1;
            bool insert_scale
                = kf.scale_itpl != Interpolation::MISSING
                && kf_i > last_scale_i + 1;
            if(insert_translation) {
                size_t i_kf_i = last_transl_i;
                for(; i_kf_i < kf_i; i_kf_i += 1) {
                    double t = (double) (i_kf_i - last_transl_i) / kf_i;
                    KeyFrame& i_kf = keyframes[i_kf_i];
                    i_kf.translation = interpolate(
                        last_transl, kf.translation, t, kf.translation_itpl
                    );
                    i_kf.translation_itpl = kf.translation_itpl;
                }
            }
            if(insert_rotation) {
                size_t i_kf_i = last_rot_i;
                for(; i_kf_i < kf_i; i_kf_i += 1) {
                    double t = (double) (i_kf_i - last_rot_i) / kf_i;
                    KeyFrame& i_kf = keyframes[i_kf_i];
                    i_kf.rotation = interpolate_spherical(
                        last_rot, kf.rotation, t, kf.rotation_itpl
                    );
                    i_kf.rotation_itpl = kf.rotation_itpl;
                }
            }
            if(insert_scale) {
                size_t i_kf_i = last_scale_i;
                for(; i_kf_i < kf_i; i_kf_i += 1) {
                    double t = (double) (i_kf_i - last_scale_i) / kf_i;
                    KeyFrame& i_kf = keyframes[i_kf_i];
                    i_kf.scale = interpolate(
                        last_scale, kf.scale, t, kf.scale_itpl
                    );
                    i_kf.scale_itpl = kf.scale_itpl;
                }
            }
            if(kf.translation_itpl != Interpolation::MISSING) {
                last_transl = kf.translation;
                last_transl_i = kf_i;
            }
            if(kf.rotation_itpl != Interpolation::MISSING) {
                last_rot = kf.rotation;
                last_rot_i = kf_i;
            }
            if(kf.scale_itpl != Interpolation::MISSING) {
                last_scale = kf.scale;
                last_scale_i = kf_i;
            }
        }
    }

    static void complete_trailing_incomplete_keyframes(
        std::vector<KeyFrame>& keyframes,
        Vec<3>& last_transl, size_t& last_transl_i,
        Vec<4>& last_rot, size_t& last_rot_i,
        Vec<3>& last_scale, size_t& last_scale_i
    ) {
        for(size_t kf_i = last_transl_i; kf_i < keyframes.size(); kf_i += 1) {
            KeyFrame& keyframe = keyframes[kf_i];
            keyframe.translation = last_transl;
            keyframe.translation_itpl = Interpolation::STEP;
        }
        for(size_t kf_i = last_rot_i; kf_i < keyframes.size(); kf_i += 1) {
            KeyFrame& keyframe = keyframes[kf_i];
            keyframe.rotation = last_rot;
            keyframe.rotation_itpl = Interpolation::STEP;
        }
        for(size_t kf_i = last_scale_i; kf_i < keyframes.size(); kf_i += 1) {
            KeyFrame& keyframe = keyframes[kf_i];
            keyframe.scale = last_scale;
            keyframe.scale_itpl = Interpolation::STEP;
        }
    }

    // Interpolates missing values of keyframe properties based on 
    // the last and next keyframe where that property is present
    //
    // All values for the first keyframes need to be present for this to
    // function correctly!
    //
    // This is needed because GLTF only specifies the values of some properties
    // at a specific keyframe, so this function fills in the remaining values
    void Animation::complete_keyframe_values() {
        size_t joint_count = this->keyframes.size();
        for(size_t joint_i = 0; joint_i < joint_count; joint_i += 1) {
            std::vector<KeyFrame>& keyframes
                = this->keyframes[joint_i];
            if(keyframes.size() == 0) { return; }
            KeyFrame& f_frame = keyframes[0];
            Vec<3> last_transl = f_frame.translation;
            size_t last_transl_i = 0;
            Vec<4> last_rot = f_frame.rotation;
            size_t last_rot_i = 0;
            Vec<3> last_scale = f_frame.scale;
            size_t last_scale_i = 0;
            complete_keyframe_gaps(
                keyframes, 
                last_transl, last_transl_i, 
                last_rot, last_rot_i,
                last_scale, last_scale_i
            );
            complete_trailing_incomplete_keyframes(
                keyframes, 
                last_transl, last_transl_i, 
                last_rot, last_rot_i,
                last_scale, last_scale_i
            );
        }
    }


    // Computes the length of the animation by getting the maximum timestamp
    // of the last keyframes of all joints.
    void Animation::compute_length() {
        this->length = 0.0;
        size_t joint_count = this->keyframes.size();
        for(size_t joint_i = 0; joint_i < joint_count; joint_i += 1) {
            std::vector<KeyFrame>& joint_frames = this->keyframes[joint_i];
            if(joint_frames.size() == 0) { continue; }
            KeyFrame& last_frame = joint_frames[joint_frames.size() - 1];
            if(this->length < last_frame.timestamp) {
                this->length = last_frame.timestamp;
            }
        }
    }


    static size_t find_next_keyframe(
        const std::vector<KeyFrame>& keyframes, double timestamp
    ) {
        for(size_t s_kf_i = 0; s_kf_i < keyframes.size(); s_kf_i += 1) {
            const KeyFrame& keyframe = keyframes[s_kf_i];
            if(keyframe.timestamp < timestamp) { continue; }
            return s_kf_i;
        }
        std::abort(); // should be unreachable
    }

    KeyFrame Animation::compute_frame(
        size_t joint_idx, double timestamp
    ) const {
        const std::vector<KeyFrame>& keyframes = this->keyframes[joint_idx];
        if(keyframes.size() == 0) { return KeyFrame(); }
        const KeyFrame& f_frame = keyframes[0];
        if(timestamp <= f_frame.timestamp) { return keyframes[0]; }
        const KeyFrame& l_frame = keyframes[keyframes.size() - 1];
        if(timestamp >= l_frame.timestamp) { return l_frame; }
        size_t next_frame_i = find_next_keyframe(keyframes, timestamp);
        size_t prev_frame_i = next_frame_i == 0? 0 : next_frame_i - 1;
        const KeyFrame& prev = keyframes[next_frame_i];
        const KeyFrame& next = keyframes[prev_frame_i];
        // copy interpolation types from the next keyframe, interpolate rest
        KeyFrame result = next;
        result.timestamp = timestamp;
        double frame_diff = next.timestamp - prev.timestamp;
        double t = (timestamp - prev.timestamp) / frame_diff;
        result.translation = interpolate(
            prev.translation, next.translation, t, result.translation_itpl
        );
        result.rotation = interpolate_spherical(
            prev.rotation, next.rotation, t, result.rotation_itpl
        );
        result.scale = interpolate(
            prev.scale, next.scale, t, result.scale_itpl
        );
        return result;
    }

}