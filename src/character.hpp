
#pragma once

#include <engine/audio.hpp>
#include "settings.hpp"
#include "renderer.hpp"
#include <functional>

namespace houseofatmos {
    
    struct CharacterAnimation {
        std::string anim_name = "";
        f64 anim_period = INFINITY;
        f64 anim_offset = 0.0;
        const engine::Sound::LoadArgs* sound = nullptr;
        f64 sound_period = INFINITY;
        f64 sound_offset = 0.0;
    };
    

    struct CharacterType {
        engine::Model::LoadArgs model;
        Mat<4> model_transform;
        Vec<3> model_heading;
        std::vector<CharacterAnimation> animations;
    };


    struct CharacterVariant {
        using TextureOverridePath = std::pair<std::string, std::string>;
        using TextureOverride = std::pair<std::string, engine::Texture>;
        
        struct LoadArgs {
            using ResourceType = CharacterVariant;

            std::vector<TextureOverridePath> texture_overrides;

            std::string pretty_identifier() const {
                std::string result = "CharacterVariant[";
                bool had_override = false;
                for(const auto& [mesh_name, path]: this->texture_overrides) {
                    if(had_override) { result += ", "; }
                    result += mesh_name + "@'" + path + "'";
                }
                return result + "]";
            }
            std::string identifier() const { return pretty_identifier(); }
        };

        std::vector<TextureOverride> texture_overrides;

        static inline CharacterVariant from_resource(const LoadArgs& args) {
            CharacterVariant variant;
            for(const auto& [mesh_name, path]: args.texture_overrides) {
                engine::Texture::LoadArgs tex_args = {
                    path, engine::Texture::vertical_mirror /* GLTF maps inverted */
                };
                variant.texture_overrides.push_back((TextureOverride) {
                    mesh_name,
                    engine::Texture::from_resource(tex_args)
                });
            }
            return variant;
        }
    };

    
    struct Character {

        struct Action {
            u64 animation_id;
            std::optional<Vec<3>> target = std::nullopt;
            f64 duration;
            f64 progress = 0.0;

            Action(u64 animation_id, f64 duration = INFINITY, f64 progress = 0.0)
                : animation_id(animation_id), duration(duration)
                , progress(progress) {}
            Action(u64 animation_id, Vec<3> target, f64 duration, f64 progress = 0.0)
                : animation_id(animation_id), target(target), duration(duration)
                , progress(progress) {}
        };

        using Behavior = std::function<void (Character&)>;

        private:
        f64 last_velocity = 0.0;
        f64 last_sound_ts = INFINITY;
        
        public:
        const CharacterType* type;
        const CharacterVariant::LoadArgs* variant;
        engine::Speaker speaker = engine::Speaker(engine::Speaker::Space::World);
        u64 default_animation_id;
        Action action;
        Vec<3> position;
        Behavior behavior;
        f64 angle = 0.0;

        Character(
            const CharacterType* type, 
            const CharacterVariant::LoadArgs* variant, 
            Vec<3> position, 
            u64 default_animation_id,
            const Settings* settings = nullptr,
            Behavior&& behavior = [](auto& c) { (void) c; }
        ): type(type), variant(variant), 
            default_animation_id(default_animation_id),
            action(Action(default_animation_id)),
            position(position), 
            behavior(std::move(behavior)) {
            if(settings != nullptr) {
                this->speaker.volume = settings->sfx_volume;
            }
        }

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

        const CharacterAnimation& animation() const {
            size_t animation_idx = (size_t) this->action.animation_id;
            return this->type->animations.at(animation_idx);
        }

        void update(
            engine::Scene& scene, const engine::Window& window,
            const Vec<3>& observer = Vec<3>(), 
            f64 update_distance = INFINITY
        ) {
            this->speaker.position = this->position;
            this->speaker.update();
            f64 distance = (this->position - observer).len();
            if(distance > update_distance) { return; }
            if(this->action.progress >= this->action.duration) {
                this->action = Action(this->default_animation_id);
            }
            this->behavior(*this);
            this->last_velocity = 1.0;
            if(this->action.target.has_value() && this->action.duration > 0.0) {
                f64 n_progress = this->action.progress / this->action.duration;
                f64 n_remaining = 1.0 - n_progress;
                f64 n_made_progress = window.delta_time() / this->action.duration;
                f64 rel_made_progress = n_made_progress < n_remaining
                    ? n_made_progress / n_remaining : 1.0; 
                Vec<3> moved_dist = (*this->action.target - this->position)
                    * rel_made_progress;
                this->position += moved_dist;
                if(rel_made_progress > 0.0) {
                    this->last_velocity = moved_dist.len() / window.delta_time();
                }
            }
            this->action.progress += window.delta_time();
            const CharacterAnimation& anim_impl = this->animation();
            if(anim_impl.sound != nullptr) {
                f64 sound_complete_c = (window.time() + anim_impl.sound_offset)
                    / (anim_impl.sound_period / this->last_velocity);
                f64 sound_ts = fmod(sound_complete_c, 1.0);
                if(sound_ts < this->last_sound_ts) {
                    this->speaker.play(scene.get(*anim_impl.sound));
                }
                this->last_sound_ts = sound_ts;
            }
        }

        void render(
            engine::Scene& scene, const engine::Window& window, 
            Renderer& renderer,
            const Vec<3>& observer = Vec<3>(), f64 draw_distance = INFINITY
        ) const {
            f64 distance = (this->position - observer).len();
            if(distance > draw_distance) { return; }
            engine::Model& model = scene.get(this->type->model);
            Mat<4> model_transf = Mat<4>::translate(this->position)
                * Mat<4>::rotate_y(this->angle)
                * this->type->model_transform;
            const CharacterAnimation& anim_impl = this->animation();
            const engine::Animation& anim = model.animation(anim_impl.anim_name);
            f64 anim_complete_c = (window.time() + anim_impl.anim_offset)
                / (anim_impl.anim_period / this->last_velocity);
            f64 anim_ts = fmod(anim_complete_c, 1.0) * anim.length();
            const CharacterVariant& variant = scene.get(*this->variant);
            for(const auto& [name, tex_overr]: variant.texture_overrides) {
                auto [primitive, unused_tex, skeleton] = model.mesh(name);
                renderer.render(
                    primitive.geometry, tex_overr, primitive.local_transform,
                    std::array { model_transf },
                    anim.compute_transformations(*skeleton, anim_ts),
                    model.face_culling
                );
            }
        }

    };

}
