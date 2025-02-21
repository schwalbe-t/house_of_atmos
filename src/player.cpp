
#include "player.hpp"

namespace houseofatmos {

    static Vec<3> get_player_heading(const engine::Window& window) {
        Vec<3> heading = { 0, 0, 0 };
        if(window.is_down(engine::Key::A)) { heading.x() -= 1; }
        if(window.is_down(engine::Key::D)) { heading.x() += 1; }
        if(window.is_down(engine::Key::W)) { heading.z() -= 1; }
        if(window.is_down(engine::Key::S)) { heading.z() += 1; }
        return heading.normalized();
    }

    static const f64 ride_speed = 10.0;
    static const f64 walk_speed = 5.0;
    static const f64 swim_speed = 2.5;

    void Player::update(const engine::Window& window) {
        Vec<3> heading = get_player_heading(window);
        f64 speed = this->is_riding? ride_speed 
            : this->in_water? swim_speed 
            : walk_speed;
        this->next_step = heading * speed * window.delta_time();
        this->confirmed_step = this->character.position;
    }

    void Player::apply_confirmed_step(
        engine::Scene& scene, const engine::Window& window
    ) {
        bool is_moving = this->next_step.len() > 0;
        this->character.action = Character::Action(
            (u64) human::Animation::Stand, window.delta_time()
        );
        if(is_moving) {
            this->character.action.target = this->confirmed_step;
            Vec<3> heading = this->confirmed_step - this->character.position;
            this->character.face_in_direction(heading);
        }
        if(this->is_riding && is_moving) {
            this->character.action.animation_id = (u64) human::Animation::HorseRide;
        } else if(this->is_riding) {
            this->character.action.animation_id = (u64) human::Animation::HorseSit;
        } else if(this->in_water && is_moving) {
            this->character.action.animation_id = (u64) human::Animation::Swim;
        } else if(this->in_water) {
            this->character.action.animation_id = (u64) human::Animation::SwimIdle;
        } else if(is_moving) {
            this->character.action.animation_id = (u64) human::Animation::Walk;
        } else { 
            this->character.action.animation_id = (u64) human::Animation::Stand;
        }
        this->character.update(scene, window);
    }

    void Player::render(
        engine::Scene& scene, const engine::Window& window, 
        Renderer& renderer
    ) {
        this->character.render(scene, window, renderer);
    }


    Player::Serialized Player::serialize() const {
        return {
            this->character.position, this->character.angle
        };
    }

}