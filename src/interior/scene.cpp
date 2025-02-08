
#include "scene.hpp"
#include <algorithm>

namespace houseofatmos::interior {

    static const Vec<3> origin = { 0, 0, 0 };

    Scene::Scene(const Interior& interior): interior(interior) {
        this->load_resources();
        this->renderer.fog_color = Vec<4>(0.0, 0.0, 0.0, 1.0); // background color
        this->renderer.lights.insert(
            this->renderer.lights.end(),
            this->interior.lights.begin(), this->interior.lights.end()
        );
        this->renderer.shadow_bias = interior.shadow_bias;
    }

    void Scene::load_resources() {
        this->load(engine::Model::Loader(this->interior.model));
        Renderer::load_shaders(*this);
        Player::load_model(*this);
    }


    bool Scene::valid_player_position(const AbsCollider& player_coll) {
        for(const Interior::Room& room: this->interior.rooms) {
            for(const RelCollider& coll: room.colliders) {
                if(!coll.at(origin).collides_with(player_coll)) { continue; }
                return true;
            }
        }
        return true;
    }

    void Scene::update(engine::Window& window) {
        this->player.update(window);
        this->player.next_step 
            = this->interior.player_velocity_matrix * this->player.next_step;
        AbsCollider next_player_x = Player::collider.at(this->player.next_x());
        if(this->valid_player_position(next_player_x)) {
            this->player.proceed_x();
        }
        AbsCollider next_player_z = Player::collider.at(this->player.next_z());
        if(this->valid_player_position(next_player_z)) {
            this->player.proceed_z();
        }
        this->renderer.camera.look_at = this->player.position;
        this->renderer.camera.position = this->player.position
            + this->interior.camera_offset;
    }


    void Scene::render_geometry(engine::Window& window, bool render_all_rooms) {
        engine::Model& model = this->get<engine::Model>(this->interior.model);
        for(const Interior::Room& room: this->interior.rooms) {
            bool room_visible = Player::collider.at(this->player.position)
                .collides_with(room.trigger.at(origin));
            if(!room_visible && !render_all_rooms) { continue; }
            auto [primitive, texture, anim] = model.mesh(room.name);
            this->renderer.render(
                primitive.geometry, texture, primitive.local_transform,
                std::array { Mat<4>() }
            );
        }
        this->player.render(*this, this->renderer);
    }

    void Scene::render(engine::Window& window) {
        this->renderer.configure(window, *this);
        this->renderer.render_to_shadow_maps();
        this->render_geometry(window, true);
        this->renderer.render_to_output();
        this->render_geometry(window, false);
        window.show_texture(this->renderer.output());
    }

}