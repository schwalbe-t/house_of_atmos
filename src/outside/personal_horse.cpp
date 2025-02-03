
#include "personal_horse.hpp"

namespace houseofatmos::outside {

    void PersonalHorse::set_free(Vec<3> at_pos) {
        this->player->is_riding = false;
        this->state = State::Idle;
        this->pos = at_pos;
        this->interactable = nullptr;
    }

    void PersonalHorse::set_called() {
        this->state = State::Called;
        this->interactable = nullptr;
    }

    void PersonalHorse::set_ridden() {
        this->player->position = this->position();
        this->player->is_riding = true;
        this->state = State::Ridden;
        this->interactable = nullptr;
    }

    void PersonalHorse::update(
        const engine::Window& window, const Terrain& terrain, Toasts& toasts
    ) {
        Vec<3> pos = this->position();
        // add interactable if in idle
        if(this->interactable == nullptr && this->state == State::Idle) {
            this->interactable = this->interactables->create([this]() {
                this->set_ridden();
            });
        }
        // set to free if riding and space bar pressed
        bool player_got_off = this->state == State::Ridden
            && window.was_pressed(engine::Key::Space);
        if(player_got_off) {
            this->set_free(pos);
        }
        // set to called if idle and H key pressed
        bool was_called = this->state == State::Idle
            && window.was_pressed(engine::Key::H);
        if(was_called) {
            this->set_called();
            toasts.add_toast("toast_personal_horse_travelling", {});
        }
        // move towards player if called
        if(this->state == State::Called) {
            Vec<3> pos_diff = this->player->position - this->pos;
            f64 remaining_dist = pos_diff.len();
            f64 speed = std::max(remaining_dist / 2.0, 5.0);
            Vec<3> heading = pos_diff.normalized();
            f64 frame_dist = speed * window.delta_time();
            if(frame_dist >= remaining_dist) {
                this->pos = this->player->position;
            } else {
                this->pos += heading * frame_dist;
            }
        }
        this->pos.y() = terrain.elevation_at(this->pos);
        // set state to idle if called and near player
        bool arrived_at_called = this->state == State::Called
            && (this->pos - this->player->position).len() < 1.0;
        if(arrived_at_called) {
            this->set_free(this->pos);
        }
        // update interactable
        if(this->interactable != nullptr) {
            this->interactable->pos = pos + Vec<3>(0, 1.0, 0.0);
        }
    }

    static inline const Vec<3> horse_model_heading = Vec<3>(0, 0, -1);

    void PersonalHorse::render(
        const Renderer& renderer, engine::Scene& scene, 
        const engine::Window& window
    ) {
        Vec<3> pos = this->position();
        Vec<3> moved = pos - this->last_pos;
        f64 moved_dist = moved.len();
        this->last_pos = pos;
        if(moved_dist > 0) {
            Vec<3> heading = moved.normalized();
            f64 angle_cross = horse_model_heading.x() * heading.z()
                - horse_model_heading.z() * heading.x();
            this->angle = atan2(angle_cross, horse_model_heading.dot(heading));
        }
        if(this->state == State::Ridden) {
            this->angle = this->player->current_angle() + pi;
        }
        engine::Model& model = scene
            .get<engine::Model>(PersonalHorse::horse_model);
        Mat<4> transform = Mat<4>::translate(this->position())
            * Mat<4>::rotate_y(this->angle);
        const engine::Animation& animation = model.animation(
            moved_dist > 0? "walk" : "idle"
        );
        f64 anim_speed = moved_dist > 0? 4.0 : 0.5;
        this->anim_timer = fmod(
            this->anim_timer + window.delta_time() * anim_speed, 
            animation.length()
        );
        const engine::Texture& texture = scene
            .get<engine::Texture>(PersonalHorse::horse_texture);
        renderer.render(
            model, std::array { transform },
            animation, this->anim_timer, 
            false, &texture
        );
    }

    PersonalHorse::Serialized PersonalHorse::serialize() const {
        return (PersonalHorse::Serialized) {
            this->position(), this->angle
        };
    }

}