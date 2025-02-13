
#pragma once

#include <engine/audio.hpp>
#include "renderer.hpp"
#include <functional>

namespace houseofatmos {
    
    struct Character {
        
        enum Animation {
            Idle = 0, // no animation
            Stand = 1, 
            Walk = 2, 
            Swim = 3, 
            HorseSit = 4, 
            HorseRide = 5,
            MaximumValue = HorseRide // = length of animations array
        };

        static inline const size_t animation_count 
            = (size_t) Animation::MaximumValue + 1;

        struct AnimationImpl {
            std::string anim_name = "";
            f64 anim_period = INFINITY;
            f64 anim_offset = 0.0;
            const engine::Sound::LoadArgs* sound = nullptr;
            f64 sound_period = INFINITY;
            f64 sound_offset = 0.0;
        };

        struct Type {
            engine::Model::LoadArgs model;
            Vec<3> model_heading;
            std::array<AnimationImpl, animation_count> animations;
        };

        struct Action {
            Animation animation;
            std::optional<Vec<3>> target = std::nullopt;
            f64 duration;
            f64 progress = 0.0;

            Action(Animation animation, f64 duration = INFINITY)
                : animation(animation), duration(duration) {}
            Action(Animation animation, Vec<3> target, f64 duration)
                : animation(animation), target(target), duration(duration) {}
        };

        using Behavior = std::function<void (Character&)>;

        private:
        f64 last_sound_ts = INFINITY;
        
        public:
        const Type* type;
        const engine::Texture::LoadArgs* variant;
        Action action = Action(Character::Idle);
        Vec<3> position;
        Behavior behavior;
        f64 angle = 0.0;

        Character(
            const Type* type, const engine::Texture::LoadArgs* variant, 
            Vec<3> position, 
            Behavior&& behavior = [](auto& c) { (void) c; }
        ): type(type), variant(variant), position(position), 
            behavior(std::move(behavior)) {}

        void face_in_direction(const Vec<3>& heading) {
            if(heading.len() == 0.0) { return; }
            Vec<3> n_heading = heading.normalized();
            f64 cross = this->type->model_heading.x() * n_heading.z()
                - this->type->model_heading.z() * n_heading.x();
            this->angle = atan2(cross, this->type->model_heading.dot(n_heading));
        }

        void face_towards(const Vec<3>& target) {
            Vec<3> heading = target - this->position;
            this->face_in_direction(heading);
        }

        const AnimationImpl& animation() const {
            size_t animation_idx = (size_t) this->action.animation;
            return this->type->animations[animation_idx];
        }

        void update(engine::Scene& scene, const engine::Window& window) {
            if(this->action.progress > this->action.duration) {
                this->action = Action(Character::Idle);
            }
            this->behavior(*this);
            f64 n_progress = this->action.progress 
                / this->action.duration;
            if(this->action.duration == 0.0) { n_progress = 1.0; }
            if(this->action.target.has_value()) {
                Vec<3> action_distance = (*this->action.target - this->position)
                    / n_progress;
                this->position = *this->action.target 
                    + action_distance * (n_progress - 1.0);
            }
            this->action.progress += window.delta_time();
            const AnimationImpl& anim_impl = this->animation();
            if(anim_impl.sound != nullptr) {
                f64 sound_complete_c = (window.time() + anim_impl.sound_offset)
                    / anim_impl.sound_period;
                f64 sound_ts = fmod(sound_complete_c, 1.0);
                if(sound_ts < this->last_sound_ts) {
                    scene.get<engine::Sound>(*anim_impl.sound).play();
                }
                this->last_sound_ts = sound_ts;
            }
        }

        void render(
            engine::Scene& scene, const engine::Window& window, 
            const Renderer& renderer
        ) {
            engine::Model& model = scene.get<engine::Model>(this->type->model);
            Mat<4> model_transf = Mat<4>::translate(this->position)
                * Mat<4>::rotate_y(this->angle);
            const AnimationImpl& anim_impl = this->animation();
            const engine::Animation& anim = model.animation(anim_impl.anim_name);
            f64 anim_complete_c = (window.time() + anim_impl.anim_offset)
                / anim_impl.anim_period;
            f64 anim_ts = fmod(anim_complete_c, 1.0) * anim.length();
            renderer.render(model, std::array { model_transf }, anim, anim_ts);
        }

    };

}
