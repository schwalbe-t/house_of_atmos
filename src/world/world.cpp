
#include "world.hpp"

namespace houseofatmos::world {

    static i64 average_area_elevation(
        const Terrain& terrain, u64 s_x, u64 s_z, u64 w, u64 h
    ) {
        u64 e_x = std::min(s_x + w, terrain.width_in_tiles());
        u64 e_z = std::min(s_z + h, terrain.width_in_tiles());
        i64 sum = 0;
        for(u64 x = s_x; x <= e_x; x += 1) {
            for(u64 z = s_z; z <= e_z; z += 1) {
                sum += terrain.elevation_at(x, z);
            }
        }
        u64 node_count = (e_x - s_x + 1) * (e_z - s_z + 1);
        return sum / (i64) node_count;
    }

    static f64 river_base_angle(f64 x, f64 z, const Terrain& terrain) {
        f64 d_left   = fabs(x - 0.0);
        f64 d_top    = fabs(z - 0.0);
        f64 d_right  = fabs(x - (f64) terrain.width_in_tiles());
        f64 d_bottom = fabs(z - (f64) terrain.height_in_tiles());
        f64 d_min_x = std::min( d_left,  d_right);
        f64 d_min_z = std::min(  d_top, d_bottom);
        f64 d_min   = std::min(d_min_x,  d_min_z);
        // 0 degress = right, clockwise (for some reason)
        if(d_left   == d_min) { return pi;       } // 180 degrees
        if(d_top    == d_min) { return pi * 1.5; } // 270 degrees
        if(d_right  == d_min) { return 0.0;      } //   0 degrees
        if(d_bottom == d_min) { return pi * 0.5; } //  90 degrees
        return 0.0; // unreachable
    }

    static void apply_river_point(u64 x, u64 z, u64 size, Terrain& terrain) {
        u64 s_x = x >= size? x - size : 0;
        u64 s_z = z >= size? z - size : 0;
        u64 e_x = std::min(s_x + size * 2, terrain.width_in_tiles());
        u64 e_z = std::min(s_z + size * 2, terrain.height_in_tiles());
        for(u64 v_x = s_x; v_x <= e_x; v_x += 1) {
            for(u64 v_z = s_z; v_z <= e_z; v_z += 1) {
                f64 center_d = (Vec<2>(v_x, v_z) - Vec<2>(x, z)).len();
                f64 river_end = (f64) size / 2.0;
                i16 max_elev = center_d <= river_end
                    ? center_d - river_end
                    : pow(center_d - river_end, 2.0);
                i16& v_elev = terrain.elevation_at(v_x, v_z);
                if(v_elev <= max_elev) { continue; }
                v_elev = max_elev;
            } 
        }
    }

    static const f64 rivers_per_sq_tile = 10.0 / (256.0 * 256.0);
    static const u64 river_min_size = 3;
    static const u64 river_max_size = 6;
    static const u64 river_size_dist = 100; // 50 steps per size

    void World::generate_rivers(StatefulRNG& rng, u32 seed) {
        u64 world_area = this->terrain.width_in_tiles() 
            * this->terrain.height_in_tiles();
        u64 river_count = (u64) ((f64) world_area * rivers_per_sq_tile);
        f64 river_spawn_area_w = (f64) this->terrain.width_in_tiles() * 0.5;
        f64 river_spawn_area_h = (f64) this->terrain.height_in_tiles() * 0.5;
        f64 river_spawn_area_x = 0.5
            * (this->terrain.width_in_tiles() - river_spawn_area_w);
        f64 river_spawn_area_z = 0.5
            * (this->terrain.height_in_tiles() - river_spawn_area_h);
        for(u64 river_i = 0; river_i < river_count; river_i += 1) {
            u64 size = (u64) (
                rng.next_f64() * (f64) (river_max_size - river_min_size)
            ) + river_min_size;
            f64 fx = rng.next_f64() * river_spawn_area_w + river_spawn_area_x;
            f64 fz = rng.next_f64() * river_spawn_area_h + river_spawn_area_z;
            f64 b_a = river_base_angle(fx, fz, this->terrain);
            for(;;) {
                if(fx < 0.0 || fz < 0.0) { break; }
                if((u64) fx >= this->terrain.width_in_tiles()) { break; }
                if((u64) fz >= this->terrain.height_in_tiles()) { break; }
                // modify terrain
                u64 x = (u64) fx;
                u64 z = (u64) fz;
                apply_river_point(x, z, size, this->terrain);
                // walk
                bool at_end = fx <= 0.0 || fz <= 0.0
                    || fx >= (f64) this->terrain.width_in_tiles()
                    || fz >= (f64) this->terrain.height_in_tiles();
                if(at_end) { break; }
                f64 angle_a = b_a + perlin_noise(seed, Vec<2>(x, z) /  4.0) * pi;
                f64 angle_b = b_a + perlin_noise(seed, Vec<2>(x, z) / 13.0) * pi;
                f64 angle_c = b_a + perlin_noise(seed, Vec<2>(x, z) / 27.0) * pi;
                Vec<2> step = (Vec<2>(cos(angle_a), sin(angle_a)) * 0.25)
                            + (Vec<2>(cos(angle_b), sin(angle_b)) * 0.50)
                            + (Vec<2>(cos(angle_c), sin(angle_c)) * 0.25);
                step = step.normalized();
                fx += step.x();
                fz += step.y();
            }
        }
    }

    static const i16 settlement_max_elevation = 10;
    static const u64 settlement_min_land_rad = 2; // in tiles
    static const f64 min_settlement_distance = 10; // in tiles

    bool World::settlement_allowed_at(
        u64 center_x, u64 center_z, const std::vector<Vec<3>> settlements
    ) {
        // center height must be less than maximum
        bool is_too_high = this->terrain.elevation_at(center_x, center_z) 
            >= settlement_max_elevation;
        if(is_too_high) { return false; }
        // all tiles in an N tile radius must be land and inside the world
        if(center_x < settlement_min_land_rad) { return false; }
        if(center_z < settlement_min_land_rad) { return false; }
        u64 start_x = center_x - settlement_min_land_rad;
        u64 start_z = center_z - settlement_min_land_rad;
        u64 end_x = center_x + settlement_min_land_rad;
        u64 end_z = center_z + settlement_min_land_rad;
        if(end_x >= this->terrain.width_in_tiles()) { return false; }
        if(end_z >= this->terrain.height_in_tiles()) { return false; }
        for(u64 x = start_x; x <= end_x; x += 1) {
            for(u64 z = start_z; z <= end_z; z += 1) {
                bool is_land = this->terrain.elevation_at(x, z) >= 0;
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

    static bool area_occupied(
        const Terrain& terrain, u64 s_x, u64 s_z, u64 w, u64 h
    ) {
        if(s_x + w > terrain.width_in_tiles()) { return true; }
        if(s_z + h > terrain.height_in_tiles()) { return true; }
        for(i64 bx = (i64) s_x - 1; bx < (i64) (s_x + w + 1); bx += 1) {
            for(i64 bz = (i64) s_z - 1; bz < (i64) (s_z + h + 1); bz += 1) {
                if(terrain.building_at(bx, bz) != nullptr) { return true; } 
            }
        }
        for(u64 x = s_x; x <= s_x + w; x += 1) {
            for(u64 z = s_z; z <= s_z + h; z += 1) {
                if(terrain.elevation_at(x, z) < 0.0) { return true; } 
            }
        }
        return false;
    }

    bool World::place_building(
        Building::Type type, u64 tile_x, u64 tile_z, 
        std::optional<ComplexId> complex
    ) {
        const Building::TypeInfo& type_info = Building::types[(size_t) type];
        bool is_occupied = area_occupied(
            this->terrain, tile_x, tile_z, type_info.width, type_info.height
        );
        if(is_occupied) { return false; }
        i64 average_height = average_area_elevation(
            this->terrain, tile_x, tile_z, type_info.width, type_info.height
        );
        for(u64 x = tile_x; x <= tile_x + type_info.width; x += 1) {
            for(u64 z = tile_z; z <= tile_z + type_info.height; z += 1) {
                this->terrain.elevation_at(x, z) = (i16) average_height;
            }
        }
        for(u64 x = tile_x; x < tile_x + type_info.width; x += 1) {
            for(u64 z = tile_z; z < tile_z + type_info.height; z += 1) {
                this->terrain.remove_foliage_at((i64) x, (i64) z);    
            }
        }
        this->terrain.adjust_area_foliage(
            (i64) tile_x - 1, (i64) tile_z - 1, 
            (i64) (tile_x + type_info.width + 1), 
            (i64) (tile_z + type_info.height + 1)
        );
        // place the building
        u64 chunk_x = tile_x / this->terrain.tiles_per_chunk();
        u64 chunk_z = tile_z / this->terrain.tiles_per_chunk();
        Terrain::ChunkData& chunk = this->terrain.chunk_at(chunk_x, chunk_z);
        u64 rel_x = tile_x % this->terrain.tiles_per_chunk();
        u64 rel_z = tile_z % this->terrain.tiles_per_chunk();
        chunk.buildings.push_back((Building) {
            type, (u8) rel_x, (u8) rel_z, complex
        });
        return true;
    }

    static const u64 settlement_min_spawn_radius = 6; // in tiles
    static const u64 settlement_max_spawn_radius = 10; // in tiles
    static const u64 pwalk_count = 4; // amount of path walks to do
    static const u64 pwalk_min_step = 3; // minimum step size of a path walker
    static const u64 pwalk_max_step = 5; // maximum step size of a path walker
    static const u64 pwalk_step_count = 20; // fairly large value

    void World::generate_settlement(
        u64 center_x, u64 center_z, StatefulRNG& rng
    ) {
        u64 spawn_radius = rng.next_u64()
            % (settlement_max_spawn_radius - settlement_min_spawn_radius)
            + settlement_min_spawn_radius;
        u64 min_x = (u64) std::max((i64) center_x - (i64) spawn_radius, (i64) 0);
        u64 min_z = (u64) std::max((i64) center_z - (i64) spawn_radius, (i64) 0);
        u64 max_x = std::min(center_x + spawn_radius, this->terrain.width_in_tiles());
        u64 max_z = std::min(center_z + spawn_radius, this->terrain.height_in_tiles());
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
                    bool on_land = this->terrain.elevation_at(curr_x, curr_z) >= 0
                        && this->terrain.elevation_at(curr_x + 1, curr_z) >= 0
                        && this->terrain.elevation_at(curr_x,     curr_z + 1) >= 0
                        && this->terrain.elevation_at(curr_x + 1, curr_z + 1) >= 0;
                    if(!on_land) { continue; }
                    u64 chunk_x = curr_x / this->terrain.tiles_per_chunk();
                    u64 chunk_z = curr_z / this->terrain.tiles_per_chunk();
                    Terrain::ChunkData& chunk = this->terrain
                        .chunk_at(chunk_x, chunk_z);
                    u64 rel_x = curr_x % this->terrain.tiles_per_chunk();
                    u64 rel_z = curr_z % this->terrain.tiles_per_chunk();
                    chunk.set_path_at(rel_x, rel_z, true);
                    this->terrain.remove_foliage_at((i64) curr_x, (i64) curr_z);
                }
                pwalker_x += step_x;
                pwalker_z += step_z;
            }
        }
        // generate plaza
        const Building::TypeInfo& plaza_info
            = Building::types[(size_t) Building::Plaza];
        ComplexId plaza_complex_i = this->complexes.create_complex();
        Complex& plaza_complex = this->complexes.get(plaza_complex_i);
        u64 plaza_x = center_x - plaza_info.width / 2;
        u64 plaza_z = center_z - plaza_info.height / 2;
        plaza_complex.add_member(plaza_x, plaza_z, Complex::Member(std::vector {
            Conversion({ { 1, Item::Beer } }, { { 5, Item::Coins } }, 0.1),
            Conversion({ { 1, Item::Bread } }, { { 3, Item::Coins } }, 0.1),
            Conversion({ { 1, Item::Armor } }, { { 60, Item::Coins } }, 0.1),
            Conversion({ { 1, Item::Tools } }, { { 30, Item::Coins } }, 0.1)
        }));
        this->place_building(
            Building::Plaza, plaza_x, plaza_z, plaza_complex_i
        );
        // generate houses
        for(u64 x = min_x; x < max_x; x += 1) {
            for(u64 z = min_z; z < max_z; z += 1) {
                bool at_path = this->terrain.path_at((i64) x - 1, (i64) z)
                    || this->terrain.path_at((i64) x + 1, (i64) z    )
                    || this->terrain.path_at((i64) x,     (i64) z - 1)
                    || this->terrain.path_at((i64) x,     (i64) z + 1);
                bool is_valid = at_path
                    && !this->terrain.path_at((i64) x, (i64) z);
                if(!is_valid) { continue; }
                this->place_building(Building::House, x, z, std::nullopt);
            }
        }
    }

    std::pair<u64, u64> World::generate_mansion() {
        i64 cx = (i64) this->terrain.width_in_tiles() / 2;
        i64 cz = (i64) this->terrain.height_in_tiles() / 2;
        f64 angle = 0.0;
        f64 dist = 0.0;
        for(;;) {
            u64 x = (u64) (cx + (i64) (cos(angle) * dist));
            u64 z = (u64) (cz + (i64) (sin(angle) * dist));
            bool placed_mansion = this
                ->place_building(Building::Mansion, x, z, std::nullopt);
            if(placed_mansion) { return { x, z }; }
            if(angle < pi * 2.0) {
                angle += pi / 12.0; // 30 degrees
                continue;
            }
            dist += 3.0;
            angle = 0.0;
        }
    }

    static const f64 terrain_falloff_distance = 10.0;
    static const f64 terrain_falloff_height = -15.0;
    static const u64 max_settlement_count = 30;
    static const u64 max_settlement_attempts = 2000;

    void World::generate_map(u32 seed) {
        auto rng = StatefulRNG(seed);
        this->terrain.generate_elevation(
            (u32) seed, terrain_falloff_distance, terrain_falloff_height
        );
        this->generate_rivers(rng, seed);
        this->terrain.generate_foliage((u32) seed);
        std::vector<Vec<3>> created_settlements;
        u64 settlement_attempts = 0;
        for(;;) {
            if(created_settlements.size() >= max_settlement_count) { break; }
            if(settlement_attempts >= max_settlement_attempts) { break; }
            u64 center_x = rng.next_u64() % this->terrain.width_in_tiles();
            u64 center_z = rng.next_u64() % this->terrain.height_in_tiles();
            bool valid_pos = this->settlement_allowed_at(
                center_x, center_z, created_settlements
            );
            if(!valid_pos) {
                settlement_attempts += 1;
                continue;
            }
            this->generate_settlement(center_x, center_z, rng);
            created_settlements.push_back(Vec<3>(center_x, 0, center_z));
        }
        if(created_settlements.size() == 0) {
            // this shouldn't happen, but in the rare case that it does
            // the map would have to be so fucked that we should probably
            // just generate a new one
            // (this is recursive, but should be fine without a base case
            //  since this should happen very very rarely)
            // (seed incremented by one to make a new map, still deterministic)
            this->generate_map(seed + 1);
        }
        auto [mansion_x, mansion_z] = this->generate_mansion();
        const Building::TypeInfo& mansion = Building::types
            .at((size_t) Building::Mansion);
        Vec<3> mansion_center_tile = Vec<3>(mansion_x, 0, mansion_z)
            + Vec<3>(mansion.width / 2.0, 0, mansion.height / 2.0);
        Vec<3> player_spawn_tile = mansion_center_tile + Vec<3>(0, 0, 0.5);
        this->player.character.position = player_spawn_tile 
            * this->terrain.units_per_tile();
        this->balance.coins = 20000;
        Vec<3> horse_spawn_pos = (player_spawn_tile + Vec<3>(-0.25, 0, 0.5))
            * this->terrain.units_per_tile();
        this->personal_horse.set_free(horse_spawn_pos);
    }


    World::World(Settings&& settings, u64 width, u64 height, u32 seed)
    : settings(std::move(settings))
    , terrain(Terrain(
        width, height, World::units_per_tile, World::tiles_per_chunk
    ))
    , carriages(CarriageManager(this->terrain)) {
        this->generate_map(seed);
    }


    World::World(Settings&& settings, const engine::Arena& buffer)
    : terrain(Terrain(0, 0, 0, 0)) {
        this->settings = std::move(settings);
        const auto& serialized = buffer.value_at<World::Serialized>(0);
        this->save_path = std::string(std::string_view(
            buffer.array_at<char>(
                serialized.save_path_offset, serialized.save_path_len
            ).data(), 
            (size_t) serialized.save_path_len
        ));
        this->terrain = Terrain(
            serialized.terrain, World::units_per_tile, World::tiles_per_chunk,
            buffer
        );
        this->complexes = ComplexBank(serialized.complexes, buffer);
        this->player = Player(serialized.player);
        this->balance = serialized.balance;
        this->carriages = CarriageManager(
            serialized.carriages, buffer, this->terrain
        );
        this->personal_horse = PersonalHorse(serialized.personal_horse);
        this->research = research::Research(serialized.research, buffer);
    }

    engine::Arena World::serialize() const {
        auto buffer = engine::Arena();
        // we need to allocate the base struct first so that it's always at offset 0
        size_t root_offset = buffer.alloc_array<World::Serialized>(nullptr, 1);
        assert(root_offset == 0);
        // we do not write to the allocated struct directly yet
        // because these serialization calls may reallocate the buffer,
        // making the reference invalid
        Terrain::Serialized terrain = this->terrain.serialize(buffer);
        ComplexBank::Serialized complexes = this->complexes.serialize(buffer);
        Player::Serialized player = this->player.serialize();
        CarriageManager::Serialized carriages = this->carriages.serialize(buffer);
        research::Research::Serialized research = this->research.serialize(buffer);
        auto& serialized = buffer.value_at<World::Serialized>(root_offset);
        serialized.save_path_len = this->save_path.size();
        serialized.save_path_offset = buffer.alloc_array<char>(
            this->save_path.data(), this->save_path.size()
        );
        serialized.terrain = terrain;
        serialized.complexes = complexes;
        serialized.player = player;
        serialized.balance = this->balance;
        serialized.carriages = carriages;
        serialized.personal_horse = this->personal_horse.serialize();
        serialized.research = research;
        return buffer;
    }

}