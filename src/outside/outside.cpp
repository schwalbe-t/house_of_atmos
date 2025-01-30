
#include "outside.hpp"
#include <fstream>

#include "terrainmap.hpp"

namespace houseofatmos::outside {

    static void load_resources(engine::Scene& scene) {
        Renderer::load_shaders(scene);
        Terrain::load_resources(scene);
        Building::load_models(scene);
        Foliage::load_models(scene);
        Player::load_model(scene);
        ActionMode::load_resources(scene);
        Carriage::load_resources(scene);
        ui::Manager::load_shaders(scene);
        ui_background::load_textures(scene);
        ui_font::load_textures(scene);
        ui_icon::load_textures(scene);
        scene.load(engine::Localization::Loader(Outside::local));
    }

    static void set_base_ui(Outside& scene, u64 selected);

    using ActionModeBuilder = std::unique_ptr<ActionMode> (*)(Outside& scene);

    static void set_action_mode(
        Outside& scene, ActionModeBuilder builder, u64 selected
    ) {
        set_base_ui(scene, selected);
        scene.action_mode = builder(scene);
    }

    static const ActionModeBuilder default_mode_const = [](Outside& s) {
        return (std::unique_ptr<ActionMode>) std::make_unique<DefaultMode>(
            s.terrain, s.complexes, s.carriages, s.player, s.balance,
            s.ui, s.toasts
        );
    };
    static const std::array<
        std::pair<const ui::Background*, ActionModeBuilder>, 4
    > action_modes = {
        (std::pair<const ui::Background*, ActionModeBuilder>) 
        { &ui_icon::terraforming, [](Outside& s) {
            return (std::unique_ptr<ActionMode>) std::make_unique<TerraformMode>(
                s.terrain, s.complexes, s.carriages, s.player, s.balance, 
                s.ui, s.toasts
            );
        } },
        { &ui_icon::construction, [](Outside& s) {
            return (std::unique_ptr<ActionMode>) std::make_unique<ConstructionMode>(
                s.terrain, s.complexes, s.carriages, s.player, s.balance, 
                s.ui, s.toasts
            );
        } },
        { &ui_icon::demolition, [](Outside& s) {
            return (std::unique_ptr<ActionMode>) std::make_unique<DemolitionMode>(
                s.terrain, s.complexes, s.carriages, s.player, s.balance,
                s.ui, s.toasts
            );
        } },
        { &ui_icon::pathing, [](Outside& s) {
            return (std::unique_ptr<ActionMode>) std::make_unique<PathingMode>(
                s.terrain, s.complexes, s.carriages, s.player, s.balance, 
                s.ui, s.toasts
            );
        } }
    };

    static void set_base_ui(Outside& scene, u64 selected) {
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
        scene.ui.root.children.push_back(std::move(mode_list));
        scene.terrain_map.create_container();
        scene.ui.root.children.push_back(ui::Element()
            .with_handle(&scene.coins_elem)
            .with_size(0, 0, ui::size::unwrapped_text)
            .with_text(
                std::to_string(scene.balance.coins) + " 🪙",
                &ui_font::standard
            )
            .with_padding(1.0)
            .with_pos(0.95, 0.05, ui::position::window_fract)
            .with_background(&ui_background::note)
            .as_movable()
        );
        scene.ui.root.children.push_back(scene.toasts.create_container());
        scene.toasts.put_states(std::move(toast_states));
    }



    static const u64 settlement_min_land_rad = 5; // in tiles
    static const f64 min_settlement_distance = 10; // in tiles

    static bool settlement_allowed_at(
        u64 center_x, u64 center_z, 
        const std::vector<Vec<3>> settlements, const Terrain& terrain
    ) {
        // all tiles in an N tile radius must be land and inside the world
        if(center_x < settlement_min_land_rad) { return false; }
        if(center_z < settlement_min_land_rad) { return false; }
        u64 start_x = center_x - settlement_min_land_rad;
        u64 start_z = center_z - settlement_min_land_rad;
        u64 end_x = center_x + settlement_min_land_rad;
        u64 end_z = center_z + settlement_min_land_rad;
        if(end_x >= terrain.width_in_tiles()) { return false; }
        if(end_z >= terrain.height_in_tiles()) { return false; }
        for(u64 x = start_x; x <= end_x; x += 1) {
            for(u64 z = start_z; z <= end_z; z += 1) {
                bool is_land = terrain.elevation_at(x, z) >= 0;
                if(!is_land) { return false; }
            }
        }
        // no other settlements nearby
        Vec<3> center = Vec<3>(center_x, 0, center_z);
        for(const Vec<3>& compared: settlements) {
            f64 distance = (compared - center).len();
            if(distance < min_settlement_distance) { return false; }
        }
        return true;
    }

    static void place_building(
        Building::Type type, u64 tile_x, u64 tile_z, 
        std::optional<ComplexId> complex, Terrain& terrain
    ) {
        const Building::TypeInfo& type_info = Building::types[(size_t) type];
        // - figure out the average height of the placed area
        // - check that all tiles are on land and none are obstructed
        i64 average_height = 0;
        if(tile_x + type_info.width >= terrain.width_in_tiles()) { return; }
        if(tile_z + type_info.height >= terrain.height_in_tiles()) { return; }
        for(u64 x = tile_x; x <= tile_x + type_info.width; x += 1) {
            for(u64 z = tile_z; z <= tile_z + type_info.height; z += 1) {
                i64 elevation = terrain.elevation_at(x, z);
                bool tile_valid = elevation >= 0
                    && !terrain.building_at((i64) x - 1, (i64) z - 1)
                    && !terrain.building_at((i64) x,     (i64) z - 1)
                    && !terrain.building_at((i64) x - 1, (i64) z    )
                    && !terrain.building_at((i64) x,     (i64) z    );
                if(!tile_valid) { return; }
                average_height += elevation;
            }
        }
        u64 node_count = (type_info.width + 1) * (type_info.height + 1);
        average_height /= node_count;
        // set the terrain heights and remove foliage
        for(u64 x = tile_x; x <= tile_x + type_info.width; x += 1) {
            for(u64 z = tile_z; z <= tile_z + type_info.height; z += 1) {
                terrain.elevation_at(x, z) = (i16) average_height;
            }
        }
        for(i64 x = (i64) tile_x - 1; (u64) x < tile_x + type_info.width + 1; x += 1) {
            for(i64 z = (i64) tile_z - 1; (u64) z < tile_z + type_info.height + 1; z += 1) {
                terrain.remove_foliage_at(x, z);    
            }
        }
        // place the building
        u64 chunk_x = tile_x / terrain.tiles_per_chunk();
        u64 chunk_z = tile_z / terrain.tiles_per_chunk();
        Terrain::ChunkData& chunk = terrain.chunk_at(chunk_x, chunk_z);
        u64 rel_x = tile_x % terrain.tiles_per_chunk();
        u64 rel_z = tile_z % terrain.tiles_per_chunk();
        chunk.buildings.push_back((Building) {
            type, (u8) rel_x, (u8) rel_z, complex
        });
    }

    static const u64 settlement_min_spawn_radius = 6; // in tiles
    static const u64 settlement_max_spawn_radius = 10; // in tiles
    static const u64 pwalk_count = 4; // amount of path walks to do
    static const u64 pwalk_min_step = 3; // minimum step size of a path walker
    static const u64 pwalk_max_step = 5; // maximum step size of a path walker
    static const u64 pwalk_step_count = 20; // fairly large value

    static void generate_settlement(
        u64 center_x, u64 center_z, StatefulRNG& rng, 
        Terrain& terrain, ComplexBank& complexes
    ) {
        u64 spawn_radius = rng.next_u64()
            % (settlement_max_spawn_radius - settlement_min_spawn_radius)
            + settlement_min_spawn_radius;
        u64 min_x = (u64) std::max((i64) center_x - (i64) spawn_radius, (i64) 0);
        u64 min_z = (u64) std::max((i64) center_z - (i64) spawn_radius, (i64) 0);
        u64 max_x = std::min(center_x + spawn_radius, terrain.width_in_tiles());
        u64 max_z = std::min(center_z + spawn_radius, terrain.height_in_tiles());
        // generate paths
        for(size_t pwalker_i = 0; pwalker_i < pwalk_count; pwalker_i += 1) {
            i64 pwalker_x = center_x;
            i64 pwalker_z = center_z;
            for(size_t step_i = 0; step_i < pwalk_step_count; step_i += 1) {
                u64 step_len = rng.next_u64() 
                    % (pwalk_max_step - pwalk_min_step) 
                    + pwalk_min_step;
                bool move_on_x = rng.next_bool();
                bool step_neg = rng.next_bool();
                i64 step_x = (!move_on_x)? 0
                    : ((i64) step_len * (step_neg? -1 : 1));
                i64 step_z = move_on_x? 0
                    : ((i64) step_len * (step_neg? -1 : 1));
                bool valid_pos = pwalker_x + step_x >= (i64) min_x 
                    && (u64) pwalker_x + (u64) step_x < max_x
                    && pwalker_z + step_z >= (i64) min_z 
                    && (u64) pwalker_z + (u64) step_z < max_z;
                if(!valid_pos) { break; }
                for(u64 abs_o = 0; abs_o < step_len; abs_o += 1) {
                    i64 o = (i64) abs_o * (step_neg? -1 : 1);
                    u64 curr_x = (u64) (pwalker_x + (move_on_x? o : 0));
                    u64 curr_z = (u64) (pwalker_z + (move_on_x? 0 : o));
                    bool on_land = terrain.elevation_at(curr_x, curr_z) >= 0
                        && terrain.elevation_at(curr_x + 1, curr_z) >= 0
                        && terrain.elevation_at(curr_x,     curr_z + 1) >= 0
                        && terrain.elevation_at(curr_x + 1, curr_z + 1) >= 0;
                    if(!on_land) { continue; }
                    u64 chunk_x = curr_x / terrain.tiles_per_chunk();
                    u64 chunk_z = curr_z / terrain.tiles_per_chunk();
                    Terrain::ChunkData& chunk = terrain
                        .chunk_at(chunk_x, chunk_z);
                    u64 rel_x = curr_x % terrain.tiles_per_chunk();
                    u64 rel_z = curr_z % terrain.tiles_per_chunk();
                    chunk.set_path_at(rel_x, rel_z, true);
                    terrain.remove_foliage_at((i64) curr_x, (i64) curr_z);
                }
                pwalker_x += step_x;
                pwalker_z += step_z;
            }
        }
        // generate plaza
        const Building::TypeInfo& plaza_info
            = Building::types[(size_t) Building::Plaza];
        ComplexId plaza_complex_i = complexes.create_complex();
        Complex& plaza_complex = complexes.get(plaza_complex_i);
        u64 plaza_x = center_x - plaza_info.width / 2;
        u64 plaza_z = center_z - plaza_info.height / 2;
        plaza_complex.add_member(plaza_x, plaza_z, Complex::Member(std::vector {
            Conversion({ { 1, Item::Beer } }, { { 5, Item::Coins } }, 0.1),
            Conversion({ { 1, Item::Bread } }, { { 3, Item::Coins } }, 0.1),
            Conversion({ { 1, Item::Armor } }, { { 60, Item::Coins } }, 0.1),
            Conversion({ { 1, Item::Tools } }, { { 30, Item::Coins } }, 0.1)
        }));
        place_building(
            Building::Plaza, plaza_x, plaza_z, plaza_complex_i, terrain
        );
        // generate houses
        for(u64 x = min_x; x < max_x; x += 1) {
            for(u64 z = min_z; z < max_z; z += 1) {
                bool at_path = terrain.path_at((i64) x - 1, (i64) z)
                    || terrain.path_at((i64) x + 1, (i64) z    )
                    || terrain.path_at((i64) x,     (i64) z - 1)
                    || terrain.path_at((i64) x,     (i64) z + 1);
                bool is_valid = at_path
                    && !terrain.path_at((i64) x, (i64) z);
                if(!is_valid) { continue; }
                place_building(Building::House, x, z, std::nullopt, terrain);
            }
        }
    }

    static const u64 max_settlement_count = 30;
    static const u64 max_settlement_attempts = 2000;

    static void generate_map(
        u32 seed, Terrain& terrain, Player& player, Balance& balance, 
        ComplexBank& complexes
    ) {
        terrain.generate_elevation((u32) seed);
        terrain.generate_foliage((u32) seed);
        auto rng = StatefulRNG(seed);
        std::vector<Vec<3>> created_settlements;
        u64 settlement_attempts = 0;
        for(;;) {
            if(created_settlements.size() >= max_settlement_count) { break; }
            if(settlement_attempts >= max_settlement_attempts) { break; }
            u64 center_x = rng.next_u64() % terrain.width_in_tiles();
            u64 center_z = rng.next_u64() % terrain.height_in_tiles();
            bool valid_pos = settlement_allowed_at(
                center_x, center_z, created_settlements, terrain
            );
            if(!valid_pos) {
                settlement_attempts += 1;
                continue;
            }
            generate_settlement(center_x, center_z, rng, terrain, complexes);
            created_settlements.push_back(Vec<3>(center_x, 0, center_z));
        }
        if(created_settlements.size() == 0) {
            // this shouldn't happen, but in the rare case that it does
            // the map would have to be so fucked that we should probably
            // just generate a new one
            // (this is recursive, but should be fine without a base case
            //  since this should happen very very rarely)
            // (seed incremented by one to make an new map, still deterministic)
            generate_map(seed + 1, terrain, player, balance, complexes);
        }
        player.position = created_settlements.at(0) * terrain.units_per_tile();
        balance.coins = 20000;
    }

    Outside::Outside() {
        load_resources(*this);
        this->carriages = CarriageManager(
            this->terrain, Outside::draw_distance_un
        );
        generate_map(
            random_init(), 
            this->terrain, this->player, this->balance, this->complexes
        );
        set_action_mode(*this, default_mode_const, UINT64_MAX);
    }



    static void save_game(engine::Arena buffer, Toasts& toasts) {
        std::ofstream fout;
        fout.open(Outside::save_location, std::ios::binary | std::ios::out);
        fout.write((const char*) buffer.data().data(), buffer.data().size());
        fout.close();
        toasts.add_toast(
            "toast_saved_game", { std::string(Outside::save_location) }
        );
    }

    static void update_player(
        engine::Window& window, Terrain& terrain, Player& player
    ) {
        player.update(window);
        bool in_coll = !terrain.valid_player_position(Player::collider.at(player.position));
        if(in_coll || terrain.valid_player_position(Player::collider.at(player.next_x()))) {
            player.proceed_x();
        }
        if(in_coll || terrain.valid_player_position(Player::collider.at(player.next_z()))) {
            player.proceed_z();
        }
        player.position.y() = std::max(
            terrain.elevation_at(player.position),
            -1.7
        );
        player.in_water = player.position.y() <= -1.5;
    }

    static void update_camera(
        engine::Window& window, Player& player, Camera& camera,
        f64& distance, const ui::Element* map
    ) {
        if(map->hidden) {
            distance += window.scrolled().y() * -5.0;
        }
        distance = std::min(
            std::max(distance, Outside::min_camera_dist), 
            Outside::max_camera_dist
        );
        camera.look_at = player.position;
        camera.position = player.position 
            + Vec<3>(0, 1, 1).normalized() * distance;
    }

    void Outside::update(engine::Window& window) {
        this->coins_elem->text = std::to_string(this->balance.coins) + " 🪙";
        this->toasts.update(window, *this);
        this->ui.update(window);
        this->carriages.update_all(
            window, this->complexes, this->terrain, this->toasts
        );
        this->complexes.update(window, this->balance);
        if(this->terrain_map.element()->hidden) {
            this->action_mode->update(window, *this, this->renderer);
        }
        update_player(window, this->terrain, this->player);
        update_camera(
            window, this->player, this->renderer.camera, this->camera_distance, 
            this->terrain_map.element()
        );
        if(window.is_down(engine::Key::LeftControl) && window.was_pressed(engine::Key::S)) {
            save_game(this->serialize(), this->toasts);
        }
        if(this->terrain_map.toggle_with_key(engine::Key::M, window)) {
            set_action_mode(*this, default_mode_const, UINT64_MAX);
            this->terrain_map.element()->hidden = false;
        }
        this->terrain_map.update(window, *this);
    }



    void Outside::render(engine::Window& window) {
        this->renderer.configure(window, *this);
        this->terrain.load_chunks_around(this->player.position);
        this->terrain.render_loaded_chunks(*this, this->renderer, window);
        this->player.render(*this, this->renderer);
        this->carriages.render_all_around(
            this->player.position, this->renderer, *this, window
        );
        this->action_mode->render(window, *this, this->renderer);
        window.show_texture(this->renderer.output());
        this->terrain_map.render();
        this->ui.render(*this, window);
        window.show_texture(this->ui.output());
    }



    Outside::Outside(const engine::Arena& buffer) {
        load_resources(*this);
        const auto& outside = buffer.value_at<Outside::Serialized>(0);
        this->terrain = Terrain(
            outside.terrain, 
            Outside::draw_distance_ch, Outside::units_per_tile, Outside::tiles_per_chunk,
            buffer
        );
        this->complexes = ComplexBank(outside.complexes, buffer);
        this->player = Player(outside.player, buffer);
        this->balance = outside.balance;
        this->carriages = CarriageManager(
            outside.carriages, buffer, this->terrain, Outside::draw_distance_un
        );
        set_action_mode(*this, default_mode_const, UINT64_MAX);
    }

    engine::Arena Outside::serialize() const {
        auto buffer = engine::Arena();
        // we need to allocate the base struct first so that it's always at offset 0
        size_t outside_offset = buffer.alloc_array<Outside::Serialized>(nullptr, 1);
        assert(outside_offset == 0);
        Terrain::Serialized terrain = this->terrain.serialize(buffer);
        ComplexBank::Serialized complexes = this->complexes.serialize(buffer);
        Player::Serialized player = this->player.serialize(buffer);
        CarriageManager::Serialized carriages = this->carriages.serialize(buffer);
        auto& outside = buffer.value_at<Outside::Serialized>(outside_offset);
        outside.terrain = terrain;
        outside.complexes = complexes;
        outside.player = player;
        outside.balance = this->balance;
        outside.carriages = carriages;
        return buffer;
    }

}