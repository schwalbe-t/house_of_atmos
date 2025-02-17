
#include "scene.hpp"
#include "../pause_menu/pause_menu.hpp"

#include "terrainmap.hpp"

namespace houseofatmos::world {

    static void set_base_ui(Scene& scene, u64 selected);

    using ActionModeBuilder = std::unique_ptr<ActionMode> (*)(Scene& scene);

    static void set_action_mode(
        Scene& scene, ActionModeBuilder builder, u64 selected
    ) {
        set_base_ui(scene, selected);
        scene.action_mode = builder(scene);
    }

    static const ActionModeBuilder default_mode_const = [](Scene& s) {
        return (std::unique_ptr<ActionMode>) std::make_unique<DefaultMode>(
            s.world, s.ui, s.toasts,
            s.get<engine::Localization>(s.world->settings.localization())
        );
    };
    static const std::array<
        std::pair<const ui::Background*, ActionModeBuilder>, 5
    > action_modes = {
        (std::pair<const ui::Background*, ActionModeBuilder>) 
        { &ui_icon::terraforming, [](Scene& s) {
            return (std::unique_ptr<ActionMode>) std::make_unique<TerraformMode>(
                s.world, s.ui, s.toasts, 
                s.get<engine::Localization>(s.world->settings.localization())
            );
        } },
        { &ui_icon::construction, [](Scene& s) {
            return (std::unique_ptr<ActionMode>) std::make_unique<ConstructionMode>(
                s.world, s.ui, s.toasts,
                s.get<engine::Localization>(s.world->settings.localization())
            );
        } },
        { &ui_icon::bridging, [](Scene& s) {
            return (std::unique_ptr<ActionMode>) std::make_unique<BridgingMode>(
                s.world, s.ui, s.toasts,
                s.get<engine::Localization>(s.world->settings.localization())
            );
        } },
        { &ui_icon::demolition, [](Scene& s) {
            return (std::unique_ptr<ActionMode>) std::make_unique<DemolitionMode>(
                s.world, s.ui, s.toasts,
                s.get<engine::Localization>(s.world->settings.localization())
            );
        } },
        { &ui_icon::pathing, [](Scene& s) {
            return (std::unique_ptr<ActionMode>) std::make_unique<PathingMode>(
                s.world, s.ui, s.toasts,
                s.get<engine::Localization>(s.world->settings.localization())
            );
        } }
    };

    static void set_base_ui(Scene& scene, u64 selected) {
        Toasts::States toast_states = scene.toasts.make_states();
        scene.ui.root.children.clear();
        ui::Element mode_list = ui::Element()
            .with_pos(0.05, 0.95, ui::position::window_fract)
            .with_size(0, 0, ui::size::units_with_children)
            .with_background(&ui_background::note)
            .with_list_dir(ui::Direction::Horizontal)
            .as_movable();
        for(u64 mode_i = 0; mode_i < action_modes.size(); mode_i += 1) {
            const auto& [mode_bkg, mode_const] = action_modes[mode_i];
            mode_list.children.push_back(ui::Element()
                .with_size(16, 16, ui::size::units)
                .with_background(mode_bkg)
                .with_click_handler([mode_i, &scene, selected, mode_const]() {
                    if(selected != mode_i) {
                        set_action_mode(scene, mode_const, mode_i);
                        return;
                    }
                    set_action_mode(scene, default_mode_const, UINT64_MAX);
                })
                .with_padding(0)
                .with_background(
                    selected == mode_i
                        ? &ui_background::border_selected
                        : &ui_background::border,
                    selected == mode_i
                        ? &ui_background::border_selected
                        : &ui_background::border_hovering
                )                
                .with_padding(2)
                .as_movable()
            );
        }
        scene.ui.root.children.push_back(scene.interactables.create_container());
        scene.ui.root.children.push_back(std::move(mode_list));
        scene.terrain_map.create_container();
        scene.ui.root.children.push_back(
            scene.world->balance.create_counter(&scene.coin_counter)
        );
        scene.toasts.set_scene(&scene);
        scene.ui.root.children.push_back(scene.toasts.create_container());
        scene.toasts.put_states(std::move(toast_states));
    }



    Scene::Scene(std::shared_ptr<World> world)
    : terrain_map(TerrainMap(
        world->settings.localization(), 
        world, this->ui
    ))
    , toasts(Toasts(world->settings.localization())) {
        this->world = world;
        this->renderer.lights.push_back(Scene::create_sun({ 0, 0, 0 }));
        this->sun = &this->renderer.lights.back();
        this->load_resources();
        set_action_mode(*this, default_mode_const, UINT64_MAX);
    }

    void Scene::load_resources() {
        Renderer::load_shaders(*this);
        Terrain::load_resources(*this);
        Building::load_models(*this);
        Foliage::load_models(*this);
        Bridge::load_models(*this);
        human::load_resources(*this);
        ActionMode::load_resources(*this);
        Carriage::load_resources(*this);
        PersonalHorse::load_resources(*this);
        ui::Manager::load_shaders(*this);
        ui_background::load_textures(*this);
        ui_const::load_all(*this);
        audio_const::load_all(*this);
        this->load(engine::Localization::Loader(
            this->world->settings.localization()
        ));
    }

    DirectionalLight Scene::create_sun(const Vec<3>& focus_point) {
        return DirectionalLight::in_direction_to(
            Vec<3>(1, -1.3, 1.2), // direction
            focus_point, 
            80.0, // radius
            200.0 // distance
        );
    }

    void Scene::configure_renderer(Renderer& renderer, u64 view_distance) {
        renderer.fog_gradiant_range = 10.0;
        renderer.fog_start_dist = view_distance
            * World::tiles_per_chunk * World::units_per_tile
            - renderer.fog_gradiant_range;
        renderer.fog_dist_scale = Vec<3>(1.0, 0.0, 1.0); // only take Y axis into account
        renderer.fog_color = Vec<4>(239, 239, 230, 255) / 255.0;
        renderer.shadow_bias = 0.0005;
        renderer.shadow_map_resolution = 4096;
    }



    static Character create_roaming_human(
        const CharacterVariant::LoadArgs& variant,
        Vec<3> origin, std::shared_ptr<StatefulRNG> rng,
        const Terrain& terrain, const engine::Window& window,
        f64 spawn_distance, f64 roam_distance, f64 roam_vel,
        const Character& player, f64 player_stop_dist
    ) {
        static const u64 no_action = UINT64_MAX;
        f64 spawn_angle = rng->next_f64() * 2.0 * pi;
        Vec<3> spawn_position = origin
            + Vec<3>(cos(spawn_angle), 0.0, sin(spawn_angle)) * spawn_distance;
        return Character(
            &human::character, &variant, spawn_position, no_action,
            [origin, rng, terrain = &terrain, window = &window, 
                roam_distance, roam_vel, 
                player = &player, player_stop_dist
            ](Character& self) {
                f64 player_dist = (player->position - self.position).len();
                bool player_is_close = player_dist <= player_stop_dist;
                bool is_walking = self.action.animation_id 
                    == (u64) human::Animation::Walk;
                // if player is close stop walking
                if(player_is_close && is_walking) {
                    self.action.animation_id = no_action;
                }
                // no walking into houses
                if(is_walking) {
                    bool current_pos_valid = terrain->valid_player_position(
                        human::collider.at(self.position), true
                    );
                    Vec<3> heading = (*self.action.target - self.position);
                    Vec<3> next_pos = self.position 
                        + heading.normalized() * roam_vel * window->delta_time();
                    bool next_pos_valid = terrain->valid_player_position(
                        human::collider.at(next_pos), true
                    );
                    if(current_pos_valid && !next_pos_valid) {
                        self.action.animation_id = no_action;
                    }
                }
                // return if the current action is still going
                if(self.action.animation_id != no_action) { return; }
                // choose a new action
                self.position.y() = terrain->elevation_at(self.position);
                if(player_is_close) {
                    self.face_towards(player->position);
                }
                if(player_is_close || rng->next_bool()) {
                    f64 duration = 1.0 + rng->next_f64() * 2.5;
                    self.action = Character::Action(
                        (u64) human::Animation::Stand, duration
                    );
                    return;
                }
                f64 walk_angle = rng->next_f64() * 2.0 * pi;
                Vec<3> to_origin = origin - self.position;
                Vec<3> walk_dir = Vec<3>(cos(walk_angle), 0.0, sin(walk_angle))
                    + to_origin.normalized() * (to_origin.len() / roam_distance);
                f64 distance = 1.0 + rng->next_f64() * 4.0;
                Vec<3> walk_dest = self.position 
                    + walk_dir.normalized() * distance;
                walk_dest.y() = terrain->elevation_at(walk_dest);
                f64 duration = distance / roam_vel;
                self.action = Character::Action(
                    (u64) human::Animation::Walk, walk_dest, duration
                );
                self.face_in_direction(walk_dir.normalized());
            }
        );
    }

    static const f64 peasant_spawn_dist = 4.0;
    static const f64 peasant_roam_dist = 15.0;
    static const f64 peasant_roam_vel = 2.5;
    static const f64 peasant_player_stop_dist = 3.0;

    static void create_peasants(
        const Terrain& terrain, const engine::Window& window, 
        const Character& player, std::vector<Character>& characters
    ) {
        auto rng = std::make_shared<StatefulRNG>();
        for(u64 chunk_x = 0; chunk_x < terrain.width_in_chunks(); chunk_x += 1) {
            for(u64 chunk_z = 0; chunk_z < terrain.height_in_chunks(); chunk_z += 1) {
                const Terrain::ChunkData& chunk = terrain.chunk_at(chunk_x, chunk_z);
                for(const Building& building: chunk.buildings) {
                    if(building.type != Building::House) { continue; }
                    if(rng->next_bool()) { continue; }
                    const Building::TypeInfo& house = building.get_type_info();
                    u64 tile_x = chunk_x * terrain.tiles_per_chunk() + building.x;
                    u64 tile_z = chunk_z * terrain.tiles_per_chunk() + building.z;
                    Vec<3> origin = Vec<3>(tile_x, 0, tile_z);
                    origin += Vec<3>(house.width, 0.0, house.height) / 2.0;
                    origin *= terrain.units_per_tile();
                    characters.push_back(create_roaming_human(
                        rng->next_bool()? human::peasant_man : human::peasant_woman,
                        origin, rng, terrain, window,
                        peasant_spawn_dist, peasant_roam_dist, peasant_roam_vel,
                        player, peasant_player_stop_dist
                    ));
                }
            }
        }
    }



    static Vec<3> player_position(Vec<3> pos, const Terrain& terrain) {
        pos.y() = std::max(
            terrain.elevation_at(pos),
            -1.7
        );
        return pos;
    }

    static void update_player(
        engine::Scene& scene, const engine::Window& window, Terrain& terrain, 
        Player& player
    ) {
        player.update(window);
        bool in_coll = !terrain.valid_player_position(
            human::collider.at(player.character.position), player.is_riding
        );
        bool next_x_free = terrain.valid_player_position(
            human::collider.at(player.next_x()), player.is_riding
        ) && terrain.valid_player_position(
            human::collider.at(player_position(player.next_x(), terrain)), 
            player.is_riding
        );
        if(in_coll || next_x_free) { player.proceed_x(); }
        bool next_z_free = terrain.valid_player_position(
            human::collider.at(player.next_z()), player.is_riding
        ) && terrain.valid_player_position(
            human::collider.at(player_position(player.next_z(), terrain)), 
            player.is_riding
        );
        if(in_coll || next_z_free) { player.proceed_z(); }
        player.apply_confirmed_step(scene, window);
        player.character.position 
            = player_position(player.character.position, terrain);
        player.in_water = player.character.position.y() <= -1.5;
    }

    static void update_camera(
        engine::Window& window, Player& player, Camera& camera,
        f64& distance, const ui::Element* map
    ) {
        if(map->hidden) {
            distance += window.scrolled().y() * -5.0;
        }
        distance = std::min(
            std::max(distance, Scene::min_camera_dist), 
            Scene::max_camera_dist
        );
        camera.look_at = player.character.position;
        camera.position = player.character.position 
            + Vec<3>(0, 1, 1).normalized() * distance;
    }

    void Scene::update(engine::Window& window) {
        this->world->settings.apply(*this, window);
        this->get<engine::Soundtrack>(audio_const::soundtrack).update();
        bool paused_game = window.was_pressed(engine::Key::Escape)
            && this->terrain_map.element()->hidden;
        if(paused_game) {
            window.set_scene(std::make_shared<PauseMenu>(
                std::shared_ptr<World>(this->world), window.scene(), 
                this->renderer.output()
            ));
        } else if(window.was_pressed(engine::Key::Escape)) {
            this->terrain_map.hide();
        }
        if(this->characters.size() == 0) {
            create_peasants(
                this->world->terrain, window, 
                this->world->player.character, this->characters
            );
        }
        this->world->carriages.update_all(
            this->world->player.character.position, this->draw_distance_units(),
            *this, window, 
            this->world->complexes, this->world->terrain, this->toasts
        );
        this->world->complexes.update(
            window, this->world->balance, this->world->research
        );
        this->world->research.check_completion(this->toasts);
        this->world->balance.update_counter(*this->coin_counter);
        this->interactables.observe_from(
            this->world->player.character.position, this->renderer, window
        );
        this->toasts.update(*this);
        this->ui.update(window);
        bool update_action_modes = !this->world->player.in_water
            && !this->world->player.is_riding
            && this->terrain_map.element()->hidden;
        if(update_action_modes) {
            this->action_mode->update(window, *this, this->renderer);
        }
        this->world->personal_horse.update(
            *this, window, this->world->terrain, this->world->player, 
            this->interactables, this->toasts
        );
        update_player(*this, window, this->world->terrain, this->world->player);
        update_camera(
            window, this->world->player, this->renderer.camera, this->camera_distance, 
            this->terrain_map.element()
        );
        for(Character& character: this->characters) {
            character.update(
                *this, window, 
                this->world->player.character.position,
                this->draw_distance_units(), 0.0
            );
        }
        if(this->terrain_map.toggle_with_key(engine::Key::M, window)) {
            set_action_mode(*this, default_mode_const, UINT64_MAX);
            this->terrain_map.element()->hidden = false;
        }
        this->terrain_map.update(window, *this);
    }



    void Scene::render_geometry(const engine::Window& window) {
        this->world->terrain.render_loaded_chunks(*this, this->renderer, window);
        this->world->player.render(*this, window, this->renderer);
        this->world->carriages.render_all_around(
            this->world->player.character.position, this->draw_distance_units(),
            this->renderer, *this, window
        );
        this->world->personal_horse.render(*this, window, this->renderer);
        for(Character& character: this->characters) {
            character.render(
                *this, window, this->renderer, 
                this->world->player.character.position, this->draw_distance_units() / 2.0
            );
        }
    }

    void Scene::render(engine::Window& window) {
        Scene::configure_renderer(
            this->renderer, this->world->settings.view_distance
        );
        *this->sun = Scene::create_sun(this->world->player.character.position);
        this->renderer.fog_origin = this->world->player.character.position;
        this->renderer.configure(window, *this);
        this->world->terrain.load_chunks_around(
            this->world->player.character.position, 
            this->world->settings.view_distance,
            &this->interactables, window, this->world
        );
        this->renderer.render_to_shadow_maps();
        this->render_geometry(window);
        this->renderer.render_to_output();
        this->render_geometry(window);
        this->world->terrain.render_water(*this, this->renderer, window);
        bool render_action_modes = !this->world->player.in_water
            && !this->world->player.is_riding
            && this->terrain_map.element()->hidden;
        if(render_action_modes) {
            this->action_mode->render(window, *this, this->renderer);
        }
        window.show_texture(this->renderer.output());
        this->terrain_map.render();
        this->ui.render(*this, window);
        window.show_texture(this->ui.output());
    }

}