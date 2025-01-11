
#include "carriage.hpp"

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
        this->curr_target_i = serialized.curr_target_i;
        this->state = serialized.state;
        this->position = serialized.position;
        this->yaw = 0.0;
        this->pitch = 0.0;
        this->travelled_dist = 0.0;
        this->moving = false;
    }


    static Vec<3> position_along_path(
        const std::vector<Vec<3>>& path, f64 distance, bool* at_end = nullptr
    ) {
        assert(path.size() > 0);
        if(path.size() == 1) { return path.front(); }
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

    void Carriage::update(
        const engine::Window& window, 
        const ComplexBank& complexes, const Terrain& terrain
    ) {
        this->moving = this->state == State::Travelling;
        if(this->state == State::Travelling) {
            // update position
            bool at_end;
            this->travelled_dist += window.delta_time() * carriage_speed;
            Vec<3> last_position = this->position;
            this->position = position_along_path(
                this->curr_path, this->travelled_dist, &at_end
            );
            this->position.y() = terrain.elevation_at(this->position);
            // compute yaw and pitch
            Vec<3> heading = (this->position - last_position).normalized();
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
            // change state if at end
            if(at_end) { this->state = State::Loading; }
        }
        if(this->state == State::Loading) {
            // TODO! transfer items and change target
            if(false /* TODO! when done loading*/) {
                this->curr_target_i += 1;
                this->curr_target_i %= this->targets.size();
                this->clear_path();
                this->state = State::Travelling;
            } 
        }
    }


    void Carriage::render(
        Renderer& renderer, engine::Scene& scene, 
        const engine::Window& window
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
                horse_animation, timestamp, false, &horse_texture
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
            carriage_animation, timestamp
        );
    }


    Carriage::Serialized Carriage::serialize(engine::Arena& buffer) const {
        return (Serialized) {
            this->type,
            this->horses.size(), buffer.alloc_array(this->horses),
            this->targets.size(), buffer.alloc_array(this->targets),
            this->curr_target_i,
            this->state,
            this->position
        };
    }



    CarriageManager::CarriageManager(const Terrain& terrain) {
        this->fill_obstacle_data(terrain);
    }

    CarriageManager::CarriageManager(
        const Serialized& serialized, const engine::Arena& buffer,
        const Terrain& terrain
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
    }


    static u64 manhattan_distance(u64 a_x, u64 a_z, u64 b_x, u64 b_z) {
        return (u64) llabs((i64) a_x - (u64) b_x) 
            + (u64) llabs((i64) a_z - (u64) b_z);
    }

    static const std::array<std::pair<i64, i64>, 4> neighbour_t_offsets = {
        std::make_pair( 0, +1),
        std::make_pair(-1,  0),
        std::make_pair(+1,  0),
        std::make_pair( 0, -1)
    };

    struct SearchedTile {
        u64 x, z;
        bool explored;
        std::optional<u64> parent;
        u64 start_dist; // manhattan distance to start tile in tiles
        u64 target_dist; // manhattan distance to target in tiles
    };

    static u64 cheapest_explorable_tile(
        const std::vector<SearchedTile>& explorable
    ) {
        u64 cheapest_cost = UINT64_MAX;
        u64 cheapest_i = UINT64_MAX;
        for(u64 search_i = 0; search_i < explorable.size(); search_i += 1) {
            const SearchedTile& searched = explorable[search_i];
            if(searched.explored) { continue; }
            u64 search_cost = searched.start_dist + searched.target_dist;
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

    static std::vector<Vec<3>> build_found_path(
        const Vec<3>& start,
        const std::vector<SearchedTile>& explorable, 
        i64 target_tile_i,
        const Terrain& terrain
    ) {
        std::vector<Vec<3>> result;
        i64 current_i = target_tile_i;
        for(;;) {
            const SearchedTile& current = explorable[current_i];
            if(!current.parent.has_value()) { break; }
            Vec<3> position = Vec<3>(current.x + 0.5, 0, current.z + 0.5)
                * terrain.units_per_tile();
            result.insert(result.begin(), position);
            current_i = *current.parent;
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
        const Building* target_b = terrain.building_at(target_x, target_z);
        assert(target_b != nullptr);
        // keep a list of tiles we can explore and add the start tile to it
        std::vector<SearchedTile> explorable;
        explorable.push_back((SearchedTile) {
            start_x, start_z, false, std::nullopt, 0, 
            manhattan_distance(start_x, start_z, target_x, target_z)
        });
        // repeatedly explore the cheapest tile
        for(;;) {
            // find the cheapest tile to explore
            u64 current_i = cheapest_explorable_tile(explorable);
            if(current_i == UINT64_MAX) { // no more tiles to explore
                return std::nullopt;
            }
            SearchedTile& current = explorable[current_i];
            current.explored = true;
            // check if the current tile is in range of the target building
            bool loadable = building_loadable_from(
                target_x, target_z, *target_b, current.x, current.z
            );
            if(loadable) {
                return build_found_path(start, explorable, current_i, terrain);
            }
            // compute data about neighbours
            for(const auto& [neighbour_ox, neighbour_oz]: neighbour_t_offsets) {
                // compute position of neighbour
                i64 neighbour_x = (i64) current.x + neighbour_ox;
                i64 neighbour_z = (i64) current.z + neighbour_oz;
                u64 neighbour_fo = (u64) neighbour_x 
                    + terrain.width_in_tiles() * (u64) neighbour_z;
                // check if neighbour is reachable
                bool reachable = neighbour_x > 0 
                    && (u64) neighbour_x < terrain.width_in_tiles()
                    && neighbour_z > 0
                    && (u64) neighbour_z < terrain.height_in_tiles()
                    && !this->obstacle_tiles[neighbour_fo];
                if(!reachable) { continue; }
                // find existing neighbour or create new
                u64 neighbour_i = explorable_tile_at(
                    explorable, (u64) neighbour_x, (u64) neighbour_z
                );
                u64 start_dist_over_current = current.start_dist
                    + abs(neighbour_ox) + abs(neighbour_oz);
                if(neighbour_i == UINT64_MAX) {
                    neighbour_i = explorable.size();
                    explorable.push_back((SearchedTile) {
                        (u64) neighbour_x, (u64) neighbour_z, false,
                        current_i, start_dist_over_current,
                        manhattan_distance(start_x, start_z, target_x, target_z)
                    });
                }
                // adjust start distance and parent of neighbour if needed
                SearchedTile& neighbour = explorable[neighbour_i];
                if(neighbour.start_dist > start_dist_over_current) {
                    neighbour.start_dist = start_dist_over_current;
                    neighbour.parent = current_i;
                }
            }
        }
    }

    void CarriageManager::find_carriage_path(
        Carriage& carriage, 
        const ComplexBank& complexes, const Terrain& terrain
    ) {
        const Complex& target = complexes.get(carriage.target().complex);
        auto path = this->find_path_to(carriage.position, target, terrain);
        if(path.has_value()) {
            carriage.set_path(*path);
        } else if(!carriage.is_lost()) {
            engine::warning("Carriage is lost!");
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
                size_t offset = x + terrain.width_in_tiles() * z;
                this->obstacle_tiles[offset] = !has_path;
            }
        }
    }

    void CarriageManager::refind_all_paths(
        const ComplexBank& complexes, const Terrain& terrain
    ) {
        this->fill_obstacle_data(terrain);
        for(Carriage& carriage: this->carriages) {
            this->find_carriage_path(carriage, complexes, terrain);
        }
    }


    void CarriageManager::update_all(
        const engine::Window& window, 
        const ComplexBank& complexes, const Terrain& terrain 
    ) {
        for(Carriage& carriage: this->carriages) {
            if(!carriage.is_lost() && !carriage.has_path()) {
                this->find_carriage_path(carriage, complexes, terrain);
            }
            carriage.update(window, complexes, terrain);
        }
    }


    void CarriageManager::render_all(
        Renderer& renderer, engine::Scene& scene, 
        const engine::Window& window
    ) {
        for(Carriage& carriage: this->carriages) {
            carriage.render(renderer, scene, window);
        }
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