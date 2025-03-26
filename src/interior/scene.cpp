
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
    ): interior(interior), 
            player(Player(world->settings)),
            toasts(Toasts(world->settings)), 
            dialogues(DialogueManager(world->settings)) {
        this->world = std::move(world);
        this->outside = std::move(outside);
        this->renderer.lights.insert(
            this->renderer.lights.end(),
            this->interior.lights.begin(), this->interior.lights.end()
        );
        this->player.character.position = interior.player_start_pos;
        this->load_resources();
        this->add_characters();
    }

    void Scene::load_resources() {
        this->load(this->interior.model);
        Renderer::load_shaders(*this);
        human::load_resources(*this);
        ui::Manager::load_shaders(*this);
        ui_const::load_all(*this);
        audio_const::load_all(*this);
        this->load(this->world->settings.localization());
    }

    void Scene::add_exit_interaction(engine::Window& window) {
        this->created_interactables.push_back(this->interactables.create(
            [this, window = &window]() {
                window->set_scene(std::shared_ptr<engine::Scene>(this->outside));
            }, 
            this->interior.exit_interactable
        ));
    }

    void Scene::add_interactions(engine::Window& window) {
        for(const auto& interaction: this->interior.interactions) {
            this->created_interactables.push_back(this->interactables.create(
                [handler = interaction.handler, this, window = &window]() {
                    handler(this->world, this->renderer, *window);
                },
                interaction.position
            ));
        }
    }

    void Scene::add_characters() {
        for(auto character_c: this->interior.characters) {
            this->characters.push_back(
                character_c(*this, this->world->settings)
            );
        }
    }

    void Scene::configure_renderer(
        Renderer& renderer, const Interior& interior, const Settings& settings
    ) {
        renderer.resolution = settings.resolution();
        renderer.fog_color = background;
        renderer.shadow_depth_bias = interior.shadow_depth_bias;
        renderer.shadow_normal_offset = interior.shadow_normal_offset;
        renderer.shadow_out_of_bounds_lit 
            = interior.shadow_out_of_bounds_lit;
        renderer.shadow_map_resolution = 512;
    }


    void Scene::init_ui(engine::Window& window) {
        Toasts::States toast_states = this->toasts.make_states();
        this->ui.root.children.clear();
        this->ui.with_element(this->interactables.create_container());
        this->add_exit_interaction(window);
        this->add_interactions(window);
        this->ui.with_element(
            this->world->balance.create_counter(&this->coin_counter)
        );
        this->toasts.set_scene(this);
        this->ui.with_element(this->toasts.create_container());
        this->toasts.put_states(std::move(toast_states));
        this->ui.with_element(this->dialogues.create_container());
    }


    bool Scene::collides_with(const AbsCollider& coll) {
        for(const Interior::Room& room: this->interior.rooms) {
            for(const RelCollider& room_coll: room.colliders) {
                if(!room_coll.at(origin).collides_with(coll)) { continue; }
                return false;
            }
        }
        return true;
    }

    void Scene::update(engine::Window& window) {
        this->world->settings.apply(*this, window);
        this->ui.unit_fract_size = this->world->settings.ui_size_fract();
        this->get(audio_const::soundtrack).update();
        if(window.was_pressed(engine::Key::Escape)) {
            window.set_scene(std::make_shared<PauseMenu>(
                std::shared_ptr<world::World>(this->world), 
                window.scene(), this->renderer.output()
            ));
        }
        if(this->ui.root.children.size() == 0) {
            this->init_ui(window);
        }
        this->dialogues.update(*this, window, this->player.character.position);
        this->player.update(window);
        this->player.next_step 
            = this->interior.player_velocity_matrix * this->player.next_step;
        AbsCollider next_player_x = human::collider.at(this->player.next_x());
        if(this->collides_with(next_player_x)) {
            this->player.proceed_x();
        }
        AbsCollider next_player_z = human::collider.at(this->player.next_z());
        if(this->collides_with(next_player_z)) {
            this->player.proceed_z();
        }
        this->player.apply_confirmed_step(*this, window);
        this->interactables.observe_from(
            this->player.character.position, this->renderer, window
        );
        this->cutscene.update(window);
        this->world->update(*this, window, this->toasts);
        this->world->balance.update_counter(*this->coin_counter);
        this->toasts.update(*this);
        this->ui.update(window);
        this->renderer.camera.look_at = this->player.character.position 
            + Vec<3>(0, 1, 0);
        this->renderer.camera.position = this->player.character.position
            + this->interior.camera_offset;
        this->listener.position = this->renderer.camera.position;
        this->listener.look_at = this->renderer.camera.look_at;
        for(auto& [character, char_update]: this->characters) {
            character.behavior = [
                this, window = &window, char_update = &char_update
            ](Character& c) {
                (*char_update)(c, *this, *window);
            };
            character.update(*this, window);
        }
    }


    void Scene::render_geometry(
        const engine::Window& window, bool render_all_rooms
    ) {
        engine::Model& model = this->get(this->interior.model);
        for(const Interior::Room& room: this->interior.rooms) {
            bool room_visible = human::collider
                .at(this->player.character.position)
                .collides_with(room.trigger.at(origin));
            if(!room_visible && !render_all_rooms) { continue; }
            auto [primitive, texture, anim] = model.mesh(std::string(room.name));
            this->renderer.render(
                primitive.geometry, texture, primitive.local_transform,
                std::array { Mat<4>() }
            );
        }
        this->player.render(*this, window, this->renderer);
        for(auto& character: this->characters) {
            character.first.render(*this, window, this->renderer);
        }
    }

    void Scene::render(engine::Window& window) {
        Scene::configure_renderer(
            this->renderer, this->interior, this->world->settings
        );
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