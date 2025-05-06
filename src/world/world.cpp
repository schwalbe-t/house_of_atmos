
#include "world.hpp"
#include <filesystem>
#include <fstream>
#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
#endif

namespace houseofatmos::world {

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
    // river generation is limited to at most have
    // as many steps as the world has tiles

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
            for(u64 step_i = 0; step_i < world_area; step_i += 1) {
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
    static const f64 min_settlement_distance = 20; // in tiles

    bool World::settlement_allowed_at(
        u64 center_x, u64 center_z, 
        const std::vector<std::pair<u64, u64>> settlements
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
        for(const auto& [comp_x, comp_z]: settlements) {
            f64 distance = (Vec<3>(comp_x, 0, comp_z) - center).len();
            if(distance < min_settlement_distance) { return false; }
        }
        return true;
    }

    static bool building_placement_allowed(
        const Terrain& terrain, u64 x, u64 z, Building::Type type
    ) {
        const Building::TypeInfo& type_info = Building::types()
            .at((size_t) type);
        return terrain.vert_area_above_water(
            x, z, x + type_info.width, z + type_info.height
        ) && terrain.vert_area_elev_mutable(
            x, z, x + type_info.width, z + type_info.height
        );
    }

    std::pair<u64, u64> World::generate_mansion() {
        i64 cx = (i64) this->terrain.width_in_tiles() / 2;
        i64 cz = (i64) this->terrain.height_in_tiles() / 2;
        f64 angle = 0.0;
        f64 dist = 0.0;
        for(;;) {
            u64 x = (u64) (cx + (i64) (cos(angle) * dist));
            u64 z = (u64) (cz + (i64) (sin(angle) * dist));
            bool allowed = building_placement_allowed(
                this->terrain, x, z, Building::Mansion
            );
            if(allowed) {
                this->terrain.place_building(Building::Mansion, x, z);
                return { x, z };
            }
            if(angle < pi * 2.0) {
                angle += pi / 12.0; // 30 degrees
                continue;
            }
            dist += 3.0;
            angle = 0.0;
        }
    }

    static const f64 terrain_falloff_distance = 40.0;
    static const f64 terrain_falloff_height = -15.0;
    static const f64 max_settlements_per_sq_tile = 15.0 / (256.0 * 256.0);
    static const u64 max_settlement_attempts = 2000;

    void World::generate_map(u32 seed) {
        auto rng = StatefulRNG(seed);
        this->terrain.generate_elevation(
            (u32) seed, terrain_falloff_distance, terrain_falloff_height
        );
        this->generate_rivers(rng, seed);
        this->terrain.generate_resources();
        this->terrain.generate_foliage((u32) seed);
        std::vector<std::pair<u64, u64>> created_settlements;
        u64 world_area = this->terrain.width_in_tiles()
            * this->terrain.height_in_tiles();
        u64 max_settlement_count = (u64) (
            max_settlements_per_sq_tile * (f64) world_area
        );
        for(u64 s_att = 0; s_att < max_settlement_attempts; s_att += 1) {
            if(created_settlements.size() >= max_settlement_count) { break; }
            u64 center_x = rng.next_u64() % this->terrain.width_in_tiles();
            u64 center_z = rng.next_u64() % this->terrain.height_in_tiles();
            bool valid_pos = this->settlement_allowed_at(
                center_x, center_z, created_settlements
            );
            if(!valid_pos) { continue; }
            PopulationName name;
            for(;;) {
                name = PopulationName(rng);
                bool exists = false;
                for(const Population& p: this->populations.populations) {
                    exists |= p.name == name;
                    if(exists) { break; }
                }
                if(!exists) { break; }
            }
            this->populations.populations
                .push_back(Population(center_x, center_z, name));
            created_settlements.push_back({ center_x, center_z });
        }
        if(created_settlements.size() == 0) {
            // this shouldn't happen, but in the rare case that it does
            // the map would have to be so fucked that we should probably
            // just generate a new one
            // (this is recursive, but should be fine without a base case
            //  since this should happen very very rarely)
            // (seed incremented by one to make a new map, still deterministic)
            this->generate_map(seed + 1);
            return;
        }
        auto [mansion_x, mansion_z] = this->generate_mansion();
        const Building::TypeInfo& mansion 
            = Building::types().at((size_t) Building::Mansion);
        Vec<3> mansion_center_tile = Vec<3>(mansion_x, 0, mansion_z)
            + Vec<3>(mansion.width / 2.0, 0, mansion.height / 2.0);
        Vec<3> player_spawn_tile = mansion_center_tile + Vec<3>(0, 0, 0.5);
        this->player.character.position = player_spawn_tile 
            * this->terrain.units_per_tile();
        this->balance.set_coins_silent(50000);
        Vec<3> horse_spawn_pos = (player_spawn_tile + Vec<3>(-0.25, 0, 0.5))
            * this->terrain.units_per_tile();
        this->personal_horse.set_free(horse_spawn_pos);
    }


    static std::function<void (PopulationManager&)> on_populations_reset(
        World* world
    ) {
        return [world](PopulationManager& populations) {
            for(const Carriage& carr: world->carriages.agents) {
                const Carriage::CarriageTypeInfo& carr_info
                    = Carriage::carriage_types().at((size_t) carr.type);
                if(!carr_info.passenger_radius.has_value()) { continue; }
                populations.register_stops(
                    carr.schedule, *carr_info.passenger_radius, 
                    world->complexes
                );
            }
            for(const Train& train: world->trains.agents) {
                const Train::LocomotiveTypeInfo& train_info
                    = Train::locomotive_types().at((size_t) train.loco_type);
                if(!train_info.passenger_radius.has_value()) { continue; }
                populations.register_stops(
                    train.schedule, *train_info.passenger_radius, 
                    world->complexes
                );
            }
            for(const Boat& boat: world->boats.agents) {
                const Boat::TypeInfo& boat_info
                    = Boat::types().at((size_t) boat.type);
                if(!boat_info.passenger_radius.has_value()) { continue; }
                populations.register_stops(
                    boat.schedule, *boat_info.passenger_radius, 
                    world->complexes
                );
            }
        };
    }


    World::World(Settings&& settings, u64 width, u64 height): 
        settings(std::move(settings)), 
        terrain(Terrain(
            width, height, World::units_per_tile, World::tiles_per_chunk
        )), 
        player(Player(this->settings)), 
        carriages(CarriageManager(
            CarriageNetwork(&this->terrain, &this->complexes)
        )), 
        trains(TrainManager(
            TrackNetwork(&this->settings, &this->terrain, &this->complexes)
        )),
        boats(BoatManager(
            BoatNetwork(&this->terrain, &this->complexes)
        )),
        personal_horse(PersonalHorse(this->settings)),
        populations(PopulationManager(on_populations_reset(this))) {
        this->carriages.reset(nullptr);
        this->trains.reset(nullptr);
        this->boats.reset(nullptr);
        this->populations.reset(this->terrain, nullptr);
    }


    World::World(Settings&& settings, const engine::Arena& buffer): 
            settings(std::move(settings)), 
            terrain(Terrain(0, 0, 0, 0)), 
            player(Player(this->settings)), 
            carriages(CarriageManager(
                CarriageNetwork(&this->terrain, &this->complexes)
            )), 
            trains(TrainManager(
                TrackNetwork(&this->settings, &this->terrain, &this->complexes)
            )),
            boats(BoatManager(
                BoatNetwork(&this->terrain, &this->complexes)
            )),
            personal_horse(PersonalHorse(this->settings)),
            populations(PopulationManager(on_populations_reset(this))) {
        const auto& serialized = buffer.get(
            engine::Arena::Position<World::Serialized>(0)
        );
        this->terrain = Terrain(
            serialized.terrain, World::units_per_tile, World::tiles_per_chunk,
            buffer
        );
        this->complexes = ComplexBank(serialized.complexes, buffer);
        this->player = Player(this->settings, serialized.player);
        this->balance = serialized.balance;
        this->carriages = CarriageManager(
            CarriageNetwork(&this->terrain, &this->complexes),
            serialized.carriages, buffer, this->settings
        );
        this->trains = TrainManager(
            TrackNetwork(&this->settings, &this->terrain, &this->complexes),
            serialized.trains, buffer, this->settings
        );
        this->boats = BoatManager(
            BoatNetwork(&this->terrain, &this->complexes),
            serialized.boats, buffer, this->settings
        );
        this->personal_horse = PersonalHorse(
            this->settings, serialized.personal_horse
        );
        this->research = research::Research(serialized.research, buffer);
        this->populations = PopulationManager(
            serialized.populations, buffer, on_populations_reset(this)
        );
        this->carriages.reset(nullptr);
        this->trains.reset(nullptr);
        this->boats.reset(nullptr);
        this->populations.reset(this->terrain, nullptr);
    }

    engine::Arena World::serialize() const {
        auto buffer = engine::Arena();
        // we need to allocate the base struct first so that it's always at offset 0
        engine::Arena::Position<World::Serialized> root_pos 
            = buffer.alloc<World::Serialized>();
        assert(root_pos.byte_offset == 0);
        // we do not get a reference to the allocated struct directly yet
        // since the following operations may cause the buffer to grow and
        // to reallocate, making the reference invalid
        Terrain::Serialized terrain = this->terrain.serialize(buffer);
        ComplexBank::Serialized complexes = this->complexes.serialize(buffer);
        Player::Serialized player = this->player.serialize();
        CarriageManager::Serialized carriages = this->carriages.serialize(buffer);
        TrainManager::Serialized trains = this->trains.serialize(buffer);
        BoatManager::Serialized boats = this->boats.serialize(buffer);
        PersonalHorse::Serialized personal_horse = this->personal_horse.serialize();
        research::Research::Serialized research = this->research.serialize(buffer);
        PopulationManager::Serialized populations = this->populations.serialize(buffer);
        auto& serialized = buffer.get(root_pos);
        serialized.format_version = World::current_format_version;
        serialized.terrain = terrain;
        serialized.complexes = complexes;
        serialized.player = player;
        serialized.balance = this->balance;
        serialized.carriages = carriages;
        serialized.trains = trains;
        serialized.boats = boats;
        serialized.personal_horse = personal_horse;
        serialized.research = research;
        serialized.populations = populations;
        return buffer;
    }

    #ifndef __EMSCRIPTEN__
        bool World::write_to_file(bool force_creation) {
            bool exists = std::filesystem::exists(this->save_path);
            bool has_path = this->save_path.size() > 0;
            bool write_allowed = this->saving_allowed 
                && has_path && (exists || force_creation);
            if(!write_allowed) { return false; }
            engine::Arena serialized = this->serialize();
            std::ofstream fout;
            fout.open(this->save_path, std::ios::binary | std::ios::out);
            fout.write(
                (const char*) serialized.data().data(), serialized.data().size()
            );
            fout.close();
            return true;
        }

        void World::trigger_autosave(
            const engine::Window& window, Toasts& toasts
        ) {
            if(!this->saving_allowed) { return; }
            f64 next_autosave = this->last_autosave_time 
                + World::autosave_period;
            if(window.time() < next_autosave) { return; }
            bool saved = this->write_to_file();
            if(saved) {
                toasts.add_toast("toast_auto_saved_game", {});
            } else {
                toasts.add_error("toast_failed_to_auto_save_game", {});
            }
            this->last_autosave_time = window.time();
        }
    #else
        EM_JS(void, hoa_save_file, (const uint8_t* data, size_t size), {
            const blob = new Blob(
                [HEAPU8.slice(data, data + size)], 
                { type: "application/octet-stream" }
            );
            const url = URL.createObjectURL(blob);
            const a = document.createElement("a");
            a.href = url;
            a.download = "savegame.bin";
            a.click();
            URL.revokeObjectURL(url);
        });

        bool World::write_to_file(bool force_creation) {
            (void) force_creation;
            this->save_path = "<download>";
            engine::Arena serialized = this->serialize();
            hoa_save_file(serialized.data().data(), serialized.data().size());
            return true;
        }

        void World::trigger_autosave(
            const engine::Window& window, Toasts& toasts
        ) {
            (void) window;
            (void) toasts;
        }
    #endif

    void World::update(
        engine::Scene& scene, const engine::Window& window, Toasts& toasts,
        ParticleManager* particles, Interactables* interactables
    ) {
        this->trigger_autosave(window, toasts);
        this->carriages.update(
            scene, window, particles, this->player, interactables
        );
        this->trains.update(
            scene, window, particles, this->player, interactables
        );
        this->boats.update(
            scene, window, particles, this->player, interactables
        );
        this->complexes.update(
            window, this->balance, this->research, this->terrain, 
            toasts, this->populations
        );
        this->populations.update(window, this->terrain, this->complexes);
    }

}