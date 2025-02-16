
#include "scene.hpp"
#include "../pause_menu/pause_menu.hpp"
#include "../world/world.hpp"
#include <algorithm>

namespace houseofatmos::interior {

    static const Vec<3> origin = { 0, 0, 0 };
    static const Vec<4> background = Vec<4>(31, 14, 28, 255) / 255.0;

    Scene::Scene(
        const Interior& interior, 
        std::shared_ptr<world::World>&& world,
            std::shared_ptr<engine::Scene>&& outside
    ): interior(interior), toasts(Toasts(world->settings.localization())) {
        this->load_resources();
        this->world = std::move(world);
        this->outside = std::move(outside);
        this->renderer.fog_color = background;
        this->renderer.lights.insert(
            this->renderer.lights.end(),
            this->interior.lights.begin(), this->interior.lights.end()
        );
        this->renderer.shadow_bias = interior.shadow_bias;
        this->renderer.shadow_map_resolution = 512;
        this->player.character.position = interior.player_start_pos;
    }

    void Scene::load_resources() {
        this->load(engine::Model::Loader(this->interior.model));
        Renderer::load_shaders(*this);
        human::load_resources(*this);
        ui::Manager::load_shaders(*this);
        ui_const::load_all(*this);
        audio_const::load_all(*this);
    }


    bool Scene::valid_player_position(const AbsCollider& player_coll) {
        for(const Interior::Room& room: this->interior.rooms) {
            for(const RelCollider& coll: room.colliders) {
                if(!coll.at(origin).collides_with(player_coll)) { continue; }
                return false;
            }
        }
        return true;
    }

    void Scene::update(engine::Window& window) {
        this->world->settings.apply(*this, window);
        this->get<engine::Soundtrack>(audio_const::soundtrack).update();
        if(window.was_pressed(engine::Key::Escape)) {
            window.set_scene(std::make_shared<PauseMenu>(
                std::shared_ptr<world::World>(this->world), 
                window.scene(), this->renderer.output()
            ));
        }
        if(this->ui.root.children.size() == 0) {
            this->ui.with_element(this->interactables.create_container());
            this->exit_interactable = this->interactables.create(
                [this, window = &window]() {
                    window->set_scene(std::shared_ptr<engine::Scene>(this->outside));
                }, 
                this->interior.exit_interactable
            );
            this->ui.root.children.push_back(
                this->world->balance.create_counter(&this->coin_counter)
            );
            this->toasts.set_scene(this);
            this->ui.root.children.push_back(this->toasts.create_container());
        }
        this->interactables.observe_from(
            this->player.character.position, this->renderer, window
        );
        this->world->balance.update_counter(*this->coin_counter);
        this->toasts.update(*this);
        this->ui.update(window);
        this->world->carriages.update_all(
            Vec<3>(0.0, 0.0, 0.0), 0.0,
            *this, window, 
            this->world->complexes, this->world->terrain, this->toasts
        );
        this->world->complexes.update(
            window, this->world->balance, this->world->research
        );
        this->world->research.check_completion(this->toasts);
        this->player.update(window);
        this->player.next_step 
            = this->interior.player_velocity_matrix * this->player.next_step;
        AbsCollider next_player_x = human::collider.at(this->player.next_x());
        if(this->valid_player_position(next_player_x)) {
            this->player.proceed_x();
        }
        AbsCollider next_player_z = human::collider.at(this->player.next_z());
        if(this->valid_player_position(next_player_z)) {
            this->player.proceed_z();
        }
        this->player.apply_confirmed_step(*this, window);
        this->renderer.camera.look_at = this->player.character.position 
            + Vec<3>(0, 1, 0);
        this->renderer.camera.position = this->player.character.position
            + this->interior.camera_offset;
    }


    void Scene::render_geometry(
        const engine::Window& window, bool render_all_rooms
    ) {
        engine::Model& model = this->get<engine::Model>(this->interior.model);
        for(const Interior::Room& room: this->interior.rooms) {
            bool room_visible = human::collider
                .at(this->player.character.position)
                .collides_with(room.trigger.at(origin));
            if(!room_visible && !render_all_rooms) { continue; }
            auto [primitive, texture, anim] = model.mesh(room.name);
            this->renderer.render(
                primitive.geometry, texture, primitive.local_transform,
                std::array { Mat<4>() }
            );
        }
        this->player.render(*this, window, this->renderer);
    }

    void Scene::render(engine::Window& window) {
        this->renderer.configure(window, *this);
        this->renderer.render_to_shadow_maps();
        this->render_geometry(window, true /* render all rooms anyway */);
        this->renderer.render_to_output();
        this->render_geometry(window, false /* render only the visible room */);
        window.show_texture(this->renderer.output());
        this->ui.render(*this, window);
        window.show_texture(this->ui.output());
    }

}