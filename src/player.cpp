
#include "player.hpp"

namespace houseofatmos {

    void Player::set_anim_idle() {
        if(this->anim_name == "idle") { return; }
        this->anim_name = "idle";
        this->anim_time = 0;
        this->anim_speed = 1;
    }

    void Player::set_anim_walk() {
        if(this->anim_name == "walk") { return; }
        this->anim_name = "walk";
        this->anim_time = 0;
        this->anim_speed = 2;
    }

    void Player::set_anim_swim() {
        if(this->anim_name == "swim") { return; }
        this->anim_name = "swim";
        this->anim_time = 0;
        this->anim_speed = 1;
    }

    void Player::set_anim_ride_idle() {
        if(this->anim_name == "ride_idle") { return; }
        this->anim_name = "ride_idle";
        this->anim_time = 0;
        this->anim_speed = 0;
    }

    void Player::set_anim_ride() {
        if(this->anim_name == "ride") { return; }
        this->anim_name = "ride";
        this->anim_time = 0;
        this->anim_speed = 3.1;
    }


    static Vec<3> get_player_heading(const engine::Window& window) {
        Vec<3> heading = { 0, 0, 0 };
        if(window.is_down(engine::Key::A)) { heading.x() -= 1; }
        if(window.is_down(engine::Key::D)) { heading.x() += 1; }
        if(window.is_down(engine::Key::W)) { heading.z() -= 1; }
        if(window.is_down(engine::Key::S)) { heading.z() += 1; }
        return heading.normalized();
    }

    static const Vec<3> model_heading = { 0, 0, 1 };
    static const f64 ride_speed = 10.0;
    static const f64 walk_speed = 5.0;
    static const f64 swim_speed = 2.5;

    void Player::update(engine::Window& window) {
        Vec<3> heading = get_player_heading(window);
        bool is_moving = heading.len() > 0;
        if(this->is_riding && is_moving) { this->set_anim_ride(); }
        else if(this->is_riding) { this->set_anim_ride_idle(); }
        else if(this->in_water) { this->set_anim_swim(); }
        else if(is_moving) { this->set_anim_walk(); }
        else { this->set_anim_idle(); }
        if(is_moving) {
            f64 cross = model_heading.x() * heading.z()
                - model_heading.z() * heading.x();
            this->angle = atan2(cross, model_heading.dot(heading));
        }
        f64 speed = this->is_riding
            ? ride_speed 
            : this->in_water? swim_speed : walk_speed;
        this->next_step = heading * speed * window.delta_time();
        this->anim_time += window.delta_time();
    }

    void Player::render(engine::Scene& scene, const Renderer& renderer) {
        engine::Model& model = scene.get<engine::Model>(Player::player_model);
        Mat<4> model_transf = Mat<4>::translate(this->position)
            * Mat<4>::rotate_y(this->angle);
        const engine::Animation& anim = model.animation(this->anim_name);
        f64 timestamp = fmod(this->anim_time * this->anim_speed, anim.length());
        renderer.render(model, std::array { model_transf }, anim, timestamp);
    }


    Player::Serialized Player::serialize(engine::Arena& buffer) const {
        (void) buffer;
        return {
            this->position, this->angle
        };
    }

}