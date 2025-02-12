
#include "carriage.hpp"
#include "../audio_const.hpp"

namespace houseofatmos::outside {

    Carriage::Carriage(
        CarriageType type, Vec<3> position,
        StatefulRNG rng
    ) {
        this->type = type;
        CarriageTypeInfo type_info = Carriage::carriage_types.at((size_t) type);
        this->horses = std::vector<HorseType>(type_info.horse_offsets.size());
        for(size_t horse_i = 0; horse_i < this->horses.size(); horse_i += 1) {
            this->horses.at(horse_i) = (HorseType) (
                rng.next_u64() % Carriage::horse_types.size()
            );
        }
        this->curr_target_i = 0;
        this->state = State::Travelling;
        this->position = position;
        this->yaw = 0.0;
        this->pitch = 0.0;
        this->travelled_dist = 0.0;
        this->load_timer = 0.0;
        this->sound_timer = 0.0;
        this->moving = false;
    }

    Carriage::Carriage(
        const Serialized& serialized, const engine::Arena& buffer
    ) {
        this->type = serialized.type;
        buffer.copy_array_at_into(
            serialized.horses_offset, serialized.horses_count,
            this->horses
        );
        buffer.copy_array_at_into(
            serialized.targets_offset, serialized.targets_count,
            this->targets
        );
        buffer.copy_map_at_into(
            serialized.items_offset, serialized.items_count,
            this->items
        );
        this->curr_target_i = serialized.curr_target_i;
        this->state = serialized.state;
        this->position = serialized.position;
        this->yaw = 0.0;
        this->pitch = 0.0;
        this->travelled_dist = 0.0;
        this->load_timer = 0.0;
        this->sound_timer = 0.0;
        this->moving = false;
    }


    static Vec<3> position_along_path(
        const std::vector<Vec<3>>& path, f64 distance, bool* at_end = nullptr
    ) {
        assert(path.size() > 0);
        if(path.size() == 1) {
            if(at_end != nullptr) { *at_end = true; }
            return path.front(); 
        }
        f64 remaining = std::max(distance, 0.0);
        for(size_t step_i = 0; step_i < path.size() - 1; step_i += 1) {
            const Vec<3>& current = path[step_i];
            const Vec<3>& next = path[step_i + 1];
            const Vec<3> step = next - current;
            f64 step_len = step.len();
            if(remaining >= step_len) {
                remaining -= step_len;
                continue;
            }
            f64 local_dist = step_len == 0? 0 : remaining / step_len;
            if(at_end != nullptr) { *at_end = false; }
            return current + step * local_dist;
        }
        if(at_end != nullptr) { *at_end = true; }
        return path.back();
    }

    static const Vec<3> model_heading = Vec<3>(0, 0, -1);
    static const f64 carriage_speed = 4.0;
    static const f64 load_time = 5.0;
    static const f64 sound_period = 0.5;
    static const f64 sound_timer_offset = 0.15;
    static const f64 alignment_points_dist = 1.0;

    static Vec<3> find_heading(
        f64 travelled_dist, const std::vector<Vec<3>>& curr_path,
        const Terrain& terrain
    ) {
        Vec<3> back = position_along_path(
            curr_path, travelled_dist - alignment_points_dist
        );
        back.y() = terrain.elevation_at(back);
        Vec<3> front = position_along_path(
            curr_path, travelled_dist + alignment_points_dist
        );
        front.y() = terrain.elevation_at(front);
        return (front - back).normalized();
    }

    void Carriage::update(
        engine::Scene& scene, const engine::Window& window, 
        ComplexBank& complexes, const Terrain& terrain,
        bool is_visible
    ) {
        this->moving = this->state == State::Travelling;
        if(this->state == State::Travelling) {
            // update position
            bool at_end = false;
            if(this->target() != nullptr) {
                // update position
                this->travelled_dist += window.delta_time() * carriage_speed;
                this->position = position_along_path(
                    this->curr_path, this->travelled_dist, &at_end
                );
                // compute yaw and pitch of the carriage
                Vec<3> heading = find_heading(
                    this->travelled_dist, this->curr_path, terrain
                );
                f64 yaw_cross = model_heading.x() * heading.z()
                    - model_heading.z() * heading.x();
                this->yaw = atan2(yaw_cross, model_heading.dot(heading));
                Vec<3> vert_heading = Vec<3>(
                    model_heading.x(),
                    heading.y(),
                    model_heading.z()
                );
                f64 pitch_cross = model_heading.y() * vert_heading.z()
                    - model_heading.z() * vert_heading.y();
                this->pitch = atan2(pitch_cross, model_heading.dot(vert_heading));
            }
            this->moving = this->target() != nullptr;
            this->position.y() = terrain.elevation_at(this->position);
            // change state and play sound if at end
            if(at_end) { this->state = State::Loading; }
            if(at_end && is_visible) {
                scene.get<engine::Sound>(sound::horse).play();
            }
            // play step sounds
            f64 next_sound_time 
                = fmod(window.time() + sound_timer_offset, sound_period);
            if(next_sound_time < this->sound_timer && is_visible) {
                scene.get<engine::Sound>(sound::step).play();
            }
            this->sound_timer = next_sound_time;
        }
        if(this->state == State::Loading) {
            this->load_timer += window.delta_time();
            if(this->load_timer >= load_time && this->targets.size() > 0) {
                // do item transfer specified in schedule
                const Target& target = this->targets[this->curr_target_i];
                Complex& complex = complexes.get(target.complex);
                u64 s_amount;
                switch(target.action) {
                    case Carriage::LoadFixed: case Carriage::LoadPercentage:
                        s_amount = complex.stored_count(target.item); break;
                    case Carriage::PutFixed: case Carriage::PutPercentage:
                        s_amount = this->stored_count(target.item); break;
                }
                u64 amount;
                switch(target.action) {
                    case Carriage::LoadFixed: case Carriage::PutFixed:
                        amount = target.amount.fixed; break;
                    case Carriage::LoadPercentage: case Carriage::PutPercentage:
                        amount = (u64) (target.amount.percentage * s_amount); 
                        break;
                }
                amount = std::max(amount, s_amount);
                switch(target.action) {
                    case Carriage::LoadFixed: case Carriage::LoadPercentage:
                        complex.remove_stored(target.item, amount);
                        this->add_stored(target.item, amount);
                        break;
                    case Carriage::PutFixed: case Carriage::PutPercentage:
                        this->remove_stored(target.item, amount);
                        complex.add_stored(target.item, amount);
                        break;
                }
                // proceed to next target
                this->curr_target_i += 1;
                this->curr_target_i %= this->targets.size();
                this->clear_path();
                this->state = State::Travelling;
                if(is_visible) {
                    scene.get<engine::Sound>(sound::horse).play();
                }
            } 
        } else {
            this->load_timer = 0.0;
        }
    }


    void Carriage::render(
        const Renderer& renderer, engine::Scene& scene, 
        const engine::Window& window,
        engine::Rendering rendering,
        const engine::Texture* override_texture
    ) {
        CarriageTypeInfo carriage_info = Carriage::carriage_types
            .at((size_t) this->type);
        // render horses
        engine::Model& horse_model = scene
            .get<engine::Model>(Carriage::horse_model);
        for(size_t horse_i = 0; horse_i < this->horses.size(); horse_i += 1) {
            HorseType horse_type = this->horses.at(horse_i);
            HorseTypeInfo horse_info = Carriage::horse_types
                .at((size_t) horse_type);
            const engine::Texture& horse_texture = scene
                .get<engine::Texture>(horse_info.texture);
            const engine::Animation& horse_animation = this->moving
                ? horse_model.animation("walk")
                : horse_model.animation("idle");
            f64 timestamp = fmod(
                window.time() * (this->moving? 2.5 : 0.5), 
                horse_animation.length()
            );
            Mat<4> horse_transform
                = Mat<4>::translate(this->position)
                * Mat<4>::rotate_y(this->yaw)
                * Mat<4>::rotate_x(this->pitch)
                * Mat<4>::translate(carriage_info.horse_offsets.at(horse_i))
                * Mat<4>::translate(carriage_info.carriage_offset);
            renderer.render(
                horse_model, std::array { horse_transform },
                horse_animation, timestamp, 
                engine::FaceCulling::Enabled,
                rendering, 
                engine::DepthTesting::Enabled,
                override_texture == nullptr? &horse_texture : override_texture
            );
        }
        // render carriage
        engine::Model& carriage_model = scene
            .get<engine::Model>(carriage_info.model);
        Mat<4> carriage_transform 
            = Mat<4>::translate(this->position)
            * Mat<4>::rotate_y(this->yaw)
            * Mat<4>::rotate_x(this->pitch)
            * Mat<4>::translate(carriage_info.carriage_offset);
        const engine::Animation& carriage_animation
            = carriage_model.animation("roll");
        f64 rolled_rotations = this->travelled_dist
            / (carriage_info.wheel_radius * 2 * pi);
        f64 timestamp = fmod(rolled_rotations, 1.0) 
            * carriage_animation.length();
        renderer.render(
            carriage_model, std::array { carriage_transform },
            carriage_animation, timestamp,
            engine::FaceCulling::Enabled,
            rendering,
            engine::DepthTesting::Enabled, 
            override_texture
        );
    }


    Carriage::Serialized Carriage::serialize(engine::Arena& buffer) const {
        return (Serialized) {
            this->type,
            this->horses.size(), buffer.alloc_array(this->horses),
            this->targets.size(), buffer.alloc_array(this->targets),
            this->items.size(), buffer.alloc_map(this->items),
            this->curr_target_i,
            this->state,
            this->position
        };
    }



    CarriageManager::CarriageManager(const Terrain& terrain, f64 draw_distance) {
        this->fill_obstacle_data(terrain);
        this->draw_distance = draw_distance;
    }

    CarriageManager::CarriageManager(
        const Serialized& serialized, const engine::Arena& buffer,
        const Terrain& terrain, f64 draw_distance
    ) {
        std::vector<Carriage::Serialized> carriages;
        buffer.copy_array_at_into(
            serialized.carriage_offset, serialized.carriage_count,
            carriages  
        );
        this->carriages.reserve(carriages.size());
        for(const Carriage::Serialized& carriage: carriages) {
            this->carriages.push_back(Carriage(carriage, buffer));
        }
        this->fill_obstacle_data(terrain);
        this->draw_distance = draw_distance;
    }


    struct Neighbour {
        i64 ox, oz;
        f64 distance;
    };

    static const std::array<Neighbour, 8> neighbour_tiles = {
        (Neighbour) { +1, +1, 1.4 },
        (Neighbour) { -1, +1, 1.4 },
        (Neighbour) { +1, -1, 1.4 },
        (Neighbour) { -1, -1, 1.4 },

        (Neighbour) {  0, +1, 1.0 },
        (Neighbour) { -1,  0, 1.0 },
        (Neighbour) { +1,  0, 1.0 },
        (Neighbour) {  0, -1, 1.0 }
    };

    struct SearchedTile {
        u64 x, z;
        bool explored;
        std::optional<u64> parent;
        f64 start_dist; // cartesian distance to start tile in tiles
        f64 target_dist; // cartesian distance to target in tiles
    };

    static u64 cheapest_explorable_tile(
        const std::vector<SearchedTile>& explorable
    ) {
        f64 cheapest_cost = UINT64_MAX;
        u64 cheapest_i = UINT64_MAX;
        for(u64 search_i = 0; search_i < explorable.size(); search_i += 1) {
            const SearchedTile& searched = explorable[search_i];
            if(searched.explored) { continue; }
            f64 search_cost = searched.start_dist + searched.target_dist;
            if(search_cost >= cheapest_cost) { continue; }
            cheapest_cost = search_cost;
            cheapest_i = search_i;
        }
        return cheapest_i;
    }

    static const u64 max_loading_distance = 1;

    static bool building_loadable_from(
        u64 building_x, u64 building_z, const Building& building,
        u64 tested_x, u64 tested_z 
    ) {
        const Building::TypeInfo& type = building.get_type_info();
        i64 start_x = (i64) building_x;
        i64 end_x = (i64) building_x + (i64) type.width - 1;
        u64 dist_x = std::max(
            (u64) std::max(start_x - (i64) tested_x, (i64) 0),
            (u64) std::max((i64) tested_x - end_x, (i64) 0)
        );
        i64 start_z = (i64) building_z;
        i64 end_z = (i64) building_z + (i64) type.height - 1;
        u64 dist_z = std::max(
            (u64) std::max(start_z - (i64) tested_z, (i64) 0),
            (u64) std::max((i64) tested_z - end_z, (i64) 0)
        );
        return (dist_x + dist_z) <= max_loading_distance;
    }

    static const f64 max_path_var = 0.05;

    static std::vector<Vec<3>> build_found_path(
        const Vec<3>& start,
        const std::vector<SearchedTile>& explorable, 
        i64 target_tile_i,
        const Terrain& terrain
    ) {
        StatefulRNG rng;
        std::vector<Vec<3>> result;
        i64 current_i = target_tile_i;
        f64 last_elev = 0;
        for(;;) {
            const SearchedTile& current = explorable[current_i];
            if(!current.parent.has_value()) { break; }
            f64 ox = 0.5 + (rng.next_f64() * 2 - 1) * max_path_var;
            f64 oz = 0.5 + (rng.next_f64() * 2 - 1) * max_path_var;
            Vec<3> position = Vec<3>(current.x + ox, 0, current.z + oz)
                * terrain.units_per_tile();
            position.y() = last_elev;
            position.y() = terrain.elevation_at(position);
            result.insert(result.begin(), position);
            current_i = *current.parent;
            last_elev = position.y();
        }
        result.insert(result.begin(), start);
        return result;
    }

    static u64 explorable_tile_at(
        const std::vector<SearchedTile>& explorable, u64 tile_x, u64 tile_z
    ) {
        for(u64 search_i = 0; search_i < explorable.size(); search_i += 1) {
            const SearchedTile& searched = explorable[search_i];
            if(searched.x != tile_x || searched.z != tile_z) { continue; }
            return search_i;
        }
        return UINT64_MAX;
    }

    std::optional<std::vector<Vec<3>>> CarriageManager::find_path_to(
        const Vec<3>& start, const Complex& target, const Terrain& terrain
    ) {
        u64 start_x = (u64) start.x() / terrain.units_per_tile();
        u64 start_z = (u64) start.z() / terrain.units_per_tile();
        auto [target_x, target_z] = target.closest_member_to(start_x, start_z);
        // the pathfinding attempts to find a way to the closest building,
        // but any building in the complex will stop the algorithm
        const Building* target_b = terrain.building_at(target_x, target_z);
        assert(target_b != nullptr);
        std::vector<std::pair<std::pair<u64, u64>, const Building*>> target_m;
        target_m.reserve(target.get_members().size());
        for(const auto& member: target.get_members()) {
            const auto& [member_x, member_z] = member.first;
            const Building* member_b = terrain.building_at(member_x, member_z);
            assert(member_b != nullptr);
            target_m.push_back({{(u64) member_x, (u64) member_z}, member_b});
        }
        // keep a list of tiles we can explore and add the start tile to it
        std::vector<SearchedTile> explorable;
        explorable.push_back((SearchedTile) {
            start_x, start_z, false, std::nullopt, 0,
            (Vec<3>(target_x, 0, target_z) - Vec<3>(start_x, 0, start_z)).len()
        });
        // repeatedly explore the cheapest tile
        for(;;) {
            // find the cheapest tile to explore
            u64 current_i = cheapest_explorable_tile(explorable);
            if(current_i == UINT64_MAX) { // no more tiles to explore
                return std::nullopt;
            }
            explorable[current_i].explored = true;
            SearchedTile current = explorable[current_i];
            // check if the current tile is in range of any target building
            bool any_loadable = false;
            for(const auto& [member_p, member_b]: target_m) {
                const auto& [member_x, member_z] = member_p;
                any_loadable |= building_loadable_from(
                    member_x, member_z, *member_b, current.x, current.z
                );
                if(any_loadable) { break; }
            }
            if(any_loadable) {
                return build_found_path(start, explorable, current_i, terrain);
            }
            // compute data about neighbours
            for(const Neighbour& neighbour: neighbour_tiles) {
                // compute position of neighbour
                i64 neighbour_x = (i64) current.x + neighbour.ox;
                i64 neighbour_z = (i64) current.z + neighbour.oz;
                u64 neighbour_fo = (u64) neighbour_x 
                    + terrain.width_in_tiles() * (u64) neighbour_z;
                f64 start_dist_over_current = current.start_dist
                    + neighbour.distance;
                // check if neighbour is reachable
                bool reachable = neighbour_x >= 0 
                    && (u64) neighbour_x < terrain.width_in_tiles()
                    && neighbour_z >= 0
                    && (u64) neighbour_z < terrain.height_in_tiles()
                    && !this->obstacle_tiles[neighbour_fo];
                if(!reachable) { continue; }
                // find existing neighbour or create new
                u64 neighbour_i = explorable_tile_at(
                    explorable, (u64) neighbour_x, (u64) neighbour_z
                );
                if(neighbour_i == UINT64_MAX) {
                    neighbour_i = explorable.size();
                    Vec<3> to_target = Vec<3>(target_x, 0, target_z)
                        - Vec<3>(neighbour_x, 0, neighbour_z);
                    explorable.push_back((SearchedTile) {
                        (u64) neighbour_x, (u64) neighbour_z, false,
                        current_i, start_dist_over_current,
                        to_target.len()
                    });
                }
                // adjust start distance and parent of neighbour if needed
                SearchedTile& neighbour_sd = explorable[neighbour_i];
                if(neighbour_sd.explored) { continue; }
                if(neighbour_sd.start_dist < start_dist_over_current) { continue; }
                neighbour_sd.start_dist = start_dist_over_current;
                neighbour_sd.parent = current_i;
            }
        }
    }

    void CarriageManager::find_carriage_path(
        Carriage& carriage, 
        const ComplexBank& complexes, const Terrain& terrain,
        Toasts& toasts
    ) {
        const Carriage::Target* target = carriage.target();
        if(target == nullptr) { return; }
        const Complex& target_c = complexes.get(target->complex);
        auto path = this->find_path_to(carriage.position, target_c, terrain);
        if(path.has_value()) {
            carriage.set_path(*path);
        } else if(!carriage.is_lost()) {
            toasts.add_error("toast_carriage_lost", {});
            carriage.make_lost();
        }
    }

    void CarriageManager::fill_obstacle_data(const Terrain& terrain) {
        size_t tile_count = terrain.width_in_tiles() * terrain.height_in_tiles();
        this->obstacle_tiles.resize(tile_count);
        for(u64 x = 0; x < terrain.width_in_tiles(); x += 1) {
            for(u64 z = 0; z < terrain.height_in_tiles(); z += 1) {
                const Terrain::ChunkData& chunk = terrain.chunk_at(
                    x / terrain.tiles_per_chunk(), z / terrain.tiles_per_chunk()
                );
                bool has_path = chunk.path_at(
                    x % terrain.tiles_per_chunk(), z % terrain.tiles_per_chunk()
                );
                bool is_bridge = terrain.bridge_at((i64) x, (i64) z) != nullptr;
                size_t offset = x + terrain.width_in_tiles() * z;
                this->obstacle_tiles[offset] = !has_path && !is_bridge;
            }
        }
        for(u64 chunk_x = 0; chunk_x < terrain.width_in_chunks(); chunk_x += 1) {
            u64 chunk_ox = chunk_x * terrain.tiles_per_chunk();
            for(u64 chunk_z = 0; chunk_z < terrain.height_in_chunks(); chunk_z += 1) {
                u64 chunk_oz = chunk_z * terrain.tiles_per_chunk();
                const Terrain::ChunkData& chunk = terrain.chunk_at(chunk_x, chunk_z);
                for(const Building& building: chunk.buildings) {
                    const Building::TypeInfo type = building.get_type_info();
                    for(u64 bx = 0; bx < type.width; bx += 1) {
                        u64 x = chunk_ox + building.x + bx;
                        if(x >= terrain.width_in_tiles()) { break; }
                        for(u64 bz = 0; bz < type.height; bz += 1) {
                            u64 z = chunk_oz + building.z + bz;
                            if(z >= terrain.height_in_tiles()) { break; }
                            size_t offset = x + terrain.width_in_tiles() * z;
                            this->obstacle_tiles[offset] = true;
                        }
                    }
                }
            }
        }
    }

    void CarriageManager::refind_all_paths(
        const ComplexBank& complexes, const Terrain& terrain, Toasts& toasts
    ) {
        this->fill_obstacle_data(terrain);
        for(Carriage& carriage: this->carriages) {
            this->find_carriage_path(carriage, complexes, terrain, toasts);
        }
    }


    void CarriageManager::update_all(
        const Vec<3>& observer, engine::Scene& scene, 
        const engine::Window& window, ComplexBank& complexes, 
        const Terrain& terrain, Toasts& toasts
    ) {
        for(Carriage& carriage: this->carriages) {
            if(!carriage.is_lost() && !carriage.has_path()) {
                this->find_carriage_path(carriage, complexes, terrain, toasts);
            }
            f64 distance = (carriage.position - observer).len();
            bool is_visible = distance <= this->draw_distance;
            carriage.update(scene, window, complexes, terrain, is_visible);
        }
    }


    void CarriageManager::render_all_around(
        const Vec<3>& observer, const Renderer& renderer, 
        engine::Scene& scene, const engine::Window& window
    ) {
        for(Carriage& carriage: this->carriages) {
            f64 distance = (carriage.position - observer).len();
            bool is_visible = distance <= this->draw_distance;
            if(!is_visible) { continue; }
            carriage.render(renderer, scene, window);
        }
    }


    static const f64 max_selection_dist = 0.2;
    
    std::optional<size_t> CarriageManager::find_selected_carriage(
        Vec<2> cursor_pos_ndc, const Renderer& renderer
    ) const {
        for(size_t carr_i = 0; carr_i < this->carriages.size(); carr_i += 1) {
            const Carriage& carriage = this->carriages[carr_i];
            Vec<2> pos_ndc = renderer.world_to_ndc(carriage.position);
            f64 dist = (pos_ndc - cursor_pos_ndc).len();
            if(dist <= max_selection_dist) { return carr_i; } 
        }
        return std::nullopt;
    }


    CarriageManager::Serialized CarriageManager::serialize(
        engine::Arena& buffer
    ) const {
        std::vector<Carriage::Serialized> carriages;
        carriages.reserve(this->carriages.size());
        for(const Carriage& carriage: this->carriages) {
            carriages.push_back(carriage.serialize(buffer));
        }
        return (Serialized) {
            this->carriages.size(), buffer.alloc_array(carriages)
        };
    }

}