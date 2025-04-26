
#include "terrain.hpp"
#include "../interior/scene.hpp"

namespace houseofatmos::world {

    static f32 water_height = -0.5;
    static f64 sand_max_height = 0.0;
    static f64 stone_min_height_diff = 2.5;


    f64 Terrain::elevation_at(const Vec<3>& pos) const {
        bool out_of_bounds = pos.x() < 0 || pos.z() < 0
            || pos.x() >= this->width_in_tiles() * this->units_per_tile()
            || pos.z() >= this->height_in_tiles() * this->units_per_tile();
        if(out_of_bounds) { return -INFINITY; }
        u64 left = (u64) (pos.x() / this->tile_size);
        u64 top = (u64) (pos.z() / this->tile_size);
        const Bridge* on_bridge = this->bridge_at((i64) left, (i64) top, pos.y());
        if(on_bridge != nullptr) {
            f64 bridge_floor = (f64) on_bridge->floor_y;
            f64 min_bridge_height = bridge_floor
                - (f64) on_bridge->get_type_info().min_height;
            if(pos.y() > min_bridge_height) { return bridge_floor; }
        }
        u64 right = left + 1;
        u64 bottom = top + 1;
        if(left >= this->width * this->tile_size) { return 0.0; }
        if(top >= this->height * this->tile_size) { return 0.0; }
        Vec<3> tl = {
            left * this->tile_size, 
            this->elevation_at(left, top), 
            top * this->tile_size 
        };
        Vec<3> tr = {
            right * this->tile_size, 
            this->elevation_at(right, top), 
            top * this->tile_size 
        };
        Vec<3> bl = { 
            left * this->tile_size, 
            this->elevation_at(left, bottom), 
            bottom * this->tile_size 
        };
        Vec<3> br = { 
            right * this->tile_size, 
            this->elevation_at(right, bottom), 
            bottom * this->tile_size 
        };
        f64 rel_z = (pos.z() - tl.z()) / this->tile_size;
        f64 l = rel_z * (bl.y() - tl.y()) + tl.y();
        f64 r = rel_z * (br.y() - tr.y()) + tr.y();
        f64 rel_x = (pos.x() - tl.x()) / this->tile_size;
        return (r - l) * rel_x + l;
    }


    static f64 compute_falloff(f64 x, f64 z, f64 width, f64 height, f64 falloff) {
        if(falloff == 0.0) { return 1.0; }
        f64 left = std::min(std::max(x / falloff, 0.0), 1.0);
        f64 top = std::min(std::max(z / falloff, 0.0), 1.0);
        f64 right = std::min(std::max((width - x) / falloff, 0.0), 1.0);
        f64 bottom = std::min(std::max((height - z) / falloff, 0.0), 1.0);
        return std::min(std::min(left, top), std::min(right, bottom));
    }

    void Terrain::generate_elevation(
        u32 seed, f64 end_falloff_distance, f64 falloff_target_height
    ) {
        f64 o = (f64) (seed % 1024);
        for(u64 x = 0; x <= this->width; x += 1) {
            for(u64 z = 0; z <= this->height; z += 1) {
                f64 n_height = 0.0;
                n_height += perlin_noise(seed, Vec<2>(o + x / 1.0, z / 1.0));
                n_height += perlin_noise(seed, Vec<2>(o + (f64) x / 75.0, (f64) z / 75.0)) * 10
                    * (perlin_noise(seed, Vec<2>(o + x / 220.0, z / 220.0)) * 5);
                n_height += 5.0;
                f64 falloff = compute_falloff(
                    (f64) x, (f64) z, (f64) this->width, (f64) this->height, 
                    end_falloff_distance
                );
                f64 f_height = n_height * falloff 
                    + falloff_target_height * (1.0 - falloff);
                this->elevation_at(x, z) = (i16) f_height;
            }
        }
    }

    void Terrain::generate_resources(u32 seed) {
        auto rng = StatefulRNG(seed);
        for(u64 tile_x = 0; tile_x < this->width; tile_x += 1) {
            for(u64 tile_z = 0; tile_z < this->height; tile_z += 1) {
                size_t r_type_count = Resource::types().size();
                for(size_t r_type_i = 0; r_type_i < r_type_count; r_type_i += 1) {
                    const Resource::TypeInfo& r_type = Resource::types()
                        .at(r_type_i);
                    bool spawned = rng.next_f64() < r_type.spawn_chance;
                    if(!spawned) { continue; }
                    bool above_water = this->vert_area_above_water(
                        tile_x, tile_z, tile_x + 1, tile_z + 1
                    );
                    if(!above_water) { continue; }
                    i16 elev = (i16) this->average_area_elevation(
                        tile_x, tile_z, 1, 1
                    );
                    this->set_area_elevation(
                        tile_x, tile_z, tile_x + 1, tile_z + 1, elev
                    );
                    u64 chunk_x = tile_x / this->tiles_per_chunk();
                    u64 chunk_z = tile_z / this->tiles_per_chunk();
                    ChunkData& chunk = this->chunk_at(chunk_x, chunk_z);
                    u64 rel_x = tile_x % this->tiles_per_chunk();
                    u64 rel_z = tile_z % this->tiles_per_chunk();
                    chunk.resources.push_back({
                        (Resource::Type) r_type_i, (u8) rel_x, (u8) rel_z
                    });
                    break;
                }
            }
        }
    }

    void Terrain::generate_foliage(u32 seed) {
        auto rng = StatefulRNG(seed);
        for(u64 tile_x = 0; tile_x < this->width; tile_x += 1) {
            for(u64 tile_z = 0; tile_z < this->height; tile_z += 1) {
                u64 chunk_x = tile_x / this->chunk_tiles;
                u64 chunk_z = tile_z / this->chunk_tiles;
                u8 rel_tile_x = (u8) (tile_x % this->chunk_tiles);
                u8 rel_tile_z = (u8) (tile_z % this->chunk_tiles);
                f64 elev_tl = this->elevation_at(tile_x,     tile_z    );
                f64 elev_tr = this->elevation_at(tile_x + 1, tile_z    );
                f64 elev_bl = this->elevation_at(tile_x,     tile_z + 1);
                f64 elev_br = this->elevation_at(tile_x + 1, tile_z + 1);
                f64 elev_min = std::min(
                    std::min(elev_tl, elev_tr), std::min(elev_bl, elev_br)
                );
                f64 elev_max = std::max(
                    std::max(elev_tl, elev_tr), std::max(elev_bl, elev_br)
                );
                if(elev_min < 0) { continue; }
                f64 height_diff = elev_max - elev_min;
                f64 slope_factor = height_diff < stone_min_height_diff? 1 : 0;
                ChunkData& chunk = this->chunk_at(chunk_x, chunk_z);
                f64 noise_val = perlin_noise(
                    seed, Vec<2>(tile_x / 25.0, tile_z / 25.0)
                ) * 0.5 + 0.5; // normalize to 0..1
                for(size_t type_i = 0; type_i < Foliage::types().size(); type_i += 1) {
                    const Foliage::TypeInfo& type = Foliage::types().at(type_i);
                    f64 chance = type.spawn_chance(noise_val) * slope_factor;
                    for(u64 att_i = 0; att_i < type.attempt_count; att_i += 1) {
                        if(rng.next_f64() >= chance) { continue; }
                        u8 x = (u8) ((rel_tile_x + rng.next_f64()) * this->tile_size);
                        u8 z = (u8) ((rel_tile_z + rng.next_f64()) * this->tile_size);
                        f32 y = (f32) this->elevation_at(Vec<3>(
                            x + chunk_x * this->chunk_tiles * this->tile_size,
                            0,
                            z + chunk_z * this->chunk_tiles * this->tile_size
                        ));
                        f32 angle = (f32) (rng.next_f64() * 2 * pi);    
                        chunk.foliage.push_back((Foliage) {
                            (Foliage::Type) type_i, x, z, y, angle
                        });
                    }
                }
            }
        }
    }


    static Vec<3> compute_normal_ccw(
        const Vec<3>& a, const Vec<3>& b, const Vec<3>& c
    ) {
        Vec<3> u = b - a;
        Vec<3> v = c - a;
        return u.cross(v).normalized();
    }

    static u16 put_terrain_vertex(
        const Vec<3>& pos, const Vec<2>& uv, const Vec<3>& normal,
        engine::Mesh& dest
    ) {
        dest.start_vertex();
        dest.put_f32({ (f32) pos.x(), (f32) pos.y(), (f32) pos.z() });
        dest.put_f32({ (f32) uv.x(), (f32) uv.y() });
        dest.put_f32({ (f32) normal.x(), (f32) normal.y(), (f32) normal.z() });
        dest.put_u8({ 0, 0, 0, 0 }); // joint indices
        dest.put_f32({ 1, 0, 0, 0 }); // joint weights
        return dest.complete_vertex();
    }

    static void put_terrain_element_ccw(
        const Vec<3>& pos_a, const Vec<3>& pos_b, const Vec<3>& pos_c, 
        const Vec<2>& uv_a, const Vec<2>& uv_b, const Vec<2>& uv_c,
        bool is_path, engine::Mesh& dest
    ) {
        f64 min_height = std::min(pos_a.y(), std::min(pos_b.y(), pos_c.y()));
        f64 max_height = std::max(pos_a.y(), std::max(pos_b.y(), pos_c.y()));
        f64 height_diff = max_height - min_height;
        Vec<2> uv_offset = is_path
            ? Vec<2>(0.505, 0.000)
            : min_height < sand_max_height
            ? Vec<2>(0.000, 0.000)
            : height_diff > stone_min_height_diff
            ? Vec<2>(0.505, 0.505)
            : Vec<2>(0.000, 0.505);
        Vec<3> normal = compute_normal_ccw(pos_a, pos_b, pos_c);
        dest.add_element(
            put_terrain_vertex(pos_a, uv_a + uv_offset, normal, dest),
            put_terrain_vertex(pos_b, uv_b + uv_offset, normal, dest),
            put_terrain_vertex(pos_c, uv_c + uv_offset, normal, dest)
        );
    }

    engine::Mesh Terrain::build_chunk_terrain_geometry(u64 chunk_x, u64 chunk_z) const {
        auto geometry = engine::Mesh(Renderer::mesh_attribs);
        const ChunkData& chunk_data = this->chunk_at(chunk_x, chunk_z);
        u64 start_x = chunk_x * this->chunk_tiles;
        u64 end_x = std::min((chunk_x + 1) * this->chunk_tiles, this->width);
        u64 start_z = chunk_z * this->chunk_tiles;
        u64 end_z = std::min((chunk_z + 1) * this->chunk_tiles, this->height);
        for(u64 x = start_x; x < end_x; x += 1) {
            for(u64 z = start_z; z < end_z; z += 1) {
                const Building* building = this->building_at((i64) x, (i64) z);
                bool skip_tile = building != nullptr
                    && !building->get_type_info().terrain_under_building;
                if(skip_tile) { continue; }
                u64 left = x - start_x;
                u64 right = left + 1;
                u64 top = z - start_z;
                u64 bottom = top + 1;
                Vec<3> pos_tl = {
                    (f32) left * this->tile_size, 
                    (f32) this->elevation_at(x, z), 
                    (f32) top * this->tile_size 
                };
                Vec<3> pos_tr = {
                    (f32) right * this->tile_size, 
                    (f32) this->elevation_at(x + 1, z), 
                    (f32) top * this->tile_size
                };
                Vec<3> pos_bl = {
                    (f32) left * this->tile_size, 
                    (f32) this->elevation_at(x, z + 1), 
                    (f32) bottom * this->tile_size
                };
                Vec<3> pos_br = {
                    (f32) right * this->tile_size,
                    (f32) this->elevation_at(x + 1, z + 1),
                    (f32) bottom * this->tile_size
                };
                f64 tl_br_height_diff = fabs(pos_tl.y() - pos_br.y());
                f64 tr_bl_height_diff = fabs(pos_tr.y() - pos_bl.y());
                f64 tl_br_height_max = std::max(pos_tl.y(), pos_br.y());
                f64 tr_bl_height_max = std::max(pos_tr.y(), pos_bl.y());
                bool is_path = chunk_data.path_at(left, top);
                Vec<2> uv_bl = { 0.00, 0.00 };
                Vec<2> uv_br = { 0.49, 0.00 };
                Vec<2> uv_tl = { 0.00, 0.49 };
                Vec<2> uv_tr = { 0.49, 0.49 };
                bool cut_tl_br = tl_br_height_diff == tr_bl_height_diff
                    ? tl_br_height_max < tr_bl_height_max
                    : tl_br_height_diff < tr_bl_height_diff;
                if(cut_tl_br) {
                    // tl---tr
                    //  | \ |
                    // bl---br
                    put_terrain_element_ccw(
                        pos_tl, pos_bl, pos_br, 
                        uv_tl, uv_bl, uv_br, 
                        is_path, geometry
                    );
                    put_terrain_element_ccw(
                        pos_tl, pos_br, pos_tr, 
                        uv_tl, uv_br, uv_tr, 
                        is_path, geometry
                    );
                } else {
                    // tl---tr
                    //  | / |
                    // bl---br
                    put_terrain_element_ccw(
                        pos_tl, pos_bl, pos_tr, 
                        uv_tl, uv_bl, uv_tr, 
                        is_path, geometry
                    );
                    put_terrain_element_ccw(
                        pos_tr, pos_bl, pos_br, 
                        uv_tr, uv_bl, uv_br, 
                        is_path, geometry
                    );
                }
            }
        }
        geometry.submit();
        return geometry;
    }

    engine::Mesh Terrain::build_chunk_water_geometry(i64 chunk_x, i64 chunk_z) const {
        auto geometry = engine::Mesh(Terrain::water_plane_attribs);
        i64 start_x = chunk_x * (i64) this->chunk_tiles;
        i64 end_x = (chunk_x + 1) * (i64) this->chunk_tiles;
        i64 start_z = chunk_z * this->chunk_tiles;
        i64 end_z = (chunk_z + 1) * (i64) this->chunk_tiles;
        for(i64 x = start_x; x < end_x; x += 1) {
            for(i64 z = start_z; z < end_z; z += 1) {
                i64 abs_t_left = x;
                i64 abs_t_right = abs_t_left + 1;
                i64 abs_t_top = z;
                i64 abs_t_bottom = abs_t_top + 1;
                bool in_bounds = x >= 0 && z >= 0
                    && x < (i64) this->width_in_tiles() 
                    && z < (i64) this->width_in_tiles();
                bool has_water = !in_bounds
                    || this->elevation_at(abs_t_left, abs_t_top) < 0.0
                    || this->elevation_at(abs_t_right, abs_t_top) < 0.0
                    || this->elevation_at(abs_t_left, abs_t_bottom) < 0.0
                    || this->elevation_at(abs_t_right, abs_t_bottom) < 0.0;
                if(!has_water) { continue; }
                f32 rel_u_left = (f32) ((abs_t_left - start_x) * this->tile_size);
                f32 rel_u_right = (f32) ((abs_t_right - start_x) * this->tile_size);
                f32 rel_u_top = (f32) ((abs_t_top - start_z) * this->tile_size);
                f32 rel_u_bottom = (f32) ((abs_t_bottom - start_z) * this->tile_size);
                // tl---tr
                //  | \ |
                // bl---br
                geometry.start_vertex();
                    geometry.put_f32({ rel_u_left, water_height, rel_u_top });
                u16 tl = geometry.complete_vertex();
                geometry.start_vertex();
                    geometry.put_f32({ rel_u_right, water_height, rel_u_top });
                u16 tr = geometry.complete_vertex();
                geometry.start_vertex();
                    geometry.put_f32({ rel_u_left, water_height, rel_u_bottom });
                u16 bl = geometry.complete_vertex();
                geometry.start_vertex();
                    geometry.put_f32({ rel_u_right, water_height, rel_u_bottom });
                u16 br = geometry.complete_vertex();
                geometry.add_element(tl, bl, br);
                geometry.add_element(tl, br, tr);
            }
        }
        geometry.submit();
        return geometry;
    }

    std::unordered_map<Foliage::Type, std::vector<Mat<4>>>
        Terrain::collect_foliage_transforms(u64 chunk_x, u64 chunk_z) const {
        std::unordered_map<Foliage::Type, std::vector<Mat<4>>> instances;
        const ChunkData& chunk_data = this->chunk_at(chunk_x, chunk_z);
        Vec<3> chunk_offset = Vec<3>(chunk_x, 0, chunk_z)
            * this->chunk_tiles * this->tile_size;
        for(const Foliage& foliage: chunk_data.foliage) {
            Vec<3> offset = chunk_offset
                + Vec<3>(foliage.x, foliage.y, foliage.z);
            Mat<4> instance = Mat<4>::translate(offset)
                * Mat<4>::rotate_y(foliage.rotation);
            instances[foliage.type].push_back(instance);
        }
        return instances;
    }

    Mat<4> Terrain::building_transform(
        const Building& building, u64 chunk_x, u64 chunk_z
    ) const {
        const Building::TypeInfo& type = building.get_type_info(); 
        Vec<3> chunk_offset_tiles = Vec<3>(chunk_x, 0, chunk_z)
            * this->tiles_per_chunk();
        Vec<3> relative_offset_tiles = Vec<3>(building.x, 0, building.z) 
            + Vec<3>(type.width / 2.0, 0, type.height / 2.0);
        Vec<3> offset = (chunk_offset_tiles + relative_offset_tiles)
            * this->units_per_tile();
        offset.y() = this->elevation_at(
            chunk_x * this->tiles_per_chunk() + building.x,
            chunk_z * this->tiles_per_chunk() + building.z
        );
        return Mat<4>::translate(offset);
    }

    std::unordered_map<Building::Type, std::vector<Mat<4>>>
        Terrain::collect_building_transforms(u64 chunk_x, u64 chunk_z) const {
        std::unordered_map<Building::Type, std::vector<Mat<4>>> instances;
        const ChunkData& chunk_data = this->chunk_at(chunk_x, chunk_z);
        for(const Building& building: chunk_data.buildings) {
            Mat<4> inst = this->building_transform(building, chunk_x, chunk_z);
            instances[building.type].push_back(inst);
        }
        return instances;
    }

    std::unordered_map<Resource::Type, std::vector<Mat<4>>>
        Terrain::collect_resource_transforms(u64 chunk_x, u64 chunk_z) const {
        std::unordered_map<Resource::Type, std::vector<Mat<4>>> instances;
        const ChunkData& chunk_data = this->chunk_at(chunk_x, chunk_z);
        u64 chunk_t_x = chunk_x * this->tiles_per_chunk();
        u64 chunk_t_z = chunk_z * this->tiles_per_chunk();
        for(const Resource& resource: chunk_data.resources) {
            u64 left = chunk_t_x + resource.x;
            u64 top = chunk_t_z + resource.z;
            u64 right = left + 1;
            u64 bottom = top + 1;
            if(chunk_data.path_at(resource.x, resource.z)) { continue; }
            if(this->building_at((i64) left, (i64) top)) { continue; }
            i64 elev = this->elevation_at(left, top);
            if(elev != this->elevation_at(right, top)) { continue; }
            if(elev != this->elevation_at(left, bottom)) { continue; }
            if(elev != this->elevation_at(right, bottom)) { continue; }
            Vec<3> offset = Vec<3>(left + 0.5, 0, top + 0.5) 
                * this->units_per_tile();
            offset.y() = elev;
            Mat<4> inst = Mat<4>::translate(offset);
            instances[resource.type].push_back(inst);
        }
        return instances;
    }

    std::unordered_map<TrackPiece::Type, std::vector<Mat<4>>>
    Terrain::collect_track_piece_transforms(u64 chunk_x, u64 chunk_z) const {
        std::unordered_map<TrackPiece::Type, std::vector<Mat<4>>> instances;
        const ChunkData& chunk_data = this->chunk_at(chunk_x, chunk_z);
        for(const TrackPiece& track_piece: chunk_data.track_pieces) {
            instances[track_piece.type].push_back(track_piece.build_transform(
                chunk_x, chunk_z, this->tiles_per_chunk(), this->units_per_tile()
            ));
        }
        return instances;
    }

    std::vector<std::shared_ptr<Interactable>> Terrain::create_chunk_interactables(
        u64 chunk_x, u64 chunk_z, 
        Interactables* interactables, engine::Window& window, 
        const std::shared_ptr<World>& world
    ) const {
        std::vector<std::shared_ptr<Interactable>> created;
        if(interactables == nullptr) { return created; }
        const ChunkData& chunk_data = this->chunk_at(chunk_x, chunk_z);
        for(const Building& building: chunk_data.buildings) {
            const Building::TypeInfo& type = building.get_type_info(); 
            if(!type.interior.has_value()) { continue; }
            u64 chunk_tile_x = chunk_x * this->tiles_per_chunk();
            u64 chunk_tile_z = chunk_z * this->tiles_per_chunk();
            f64 pos_tile_x = chunk_tile_x + building.x + type.width / 2.0;
            f64 pos_tile_z = chunk_tile_z + building.z + type.height/ 2.0;
            Vec<3> position = Vec<3>(pos_tile_x, 0.0, pos_tile_z)
                * this->units_per_tile();
            position.y() += this->elevation_at(
                chunk_tile_x + building.x, chunk_tile_z + building.z
            );
            position += type.interior->interactable_offset;
            created.push_back(interactables->create(
                [window = &window, type = &type, 
                    world = std::shared_ptr<World>(world)
                ]() {
                    window->set_scene(std::make_shared<interior::Scene>(
                        type->interior->interior, std::shared_ptr<World>(world), 
                        window->scene()
                    ));
                },
                position
            ));
        }
        return created;
    }

    std::vector<ParticleSpawner> Terrain::create_chunk_particle_spawners(
        u64 chunk_x, u64 chunk_z
    ) {
        std::vector<ParticleSpawner> spawners;
        const ChunkData& chunk = this->chunk_at(chunk_x, chunk_z);
        for(const Building& building: chunk.buildings) {
            const Building::TypeInfo& b_info = Building::types()
                .at((size_t) building.type);
            if(!b_info.particle_spawner.has_value()) { continue; }
            u64 tile_x = chunk_x * this->chunk_tiles + building.x;
            u64 tile_z = chunk_z * this->chunk_tiles + building.z;
            Vec<3> tile_pos = Vec<3>(tile_x, 0, tile_z)
                + Vec<3>(b_info.width, 0, b_info.height) * 0.5;
            Vec<3> position = tile_pos * this->tile_size;
            position.y() = this->elevation_at(tile_x, tile_z);
            spawners.push_back((*b_info.particle_spawner)(position, this->rng));
        }
        for(const Foliage& foliage: chunk.foliage) {
            const Foliage::TypeInfo& f_info = Foliage::types()
                .at((size_t) foliage.type);
            if(!f_info.particle_spawner.has_value()) { continue; }
            u64 ch_pos_x = chunk_x * this->chunk_tiles * this->tile_size;
            u64 ch_pos_z = chunk_z * this->chunk_tiles * this->tile_size;
            Vec<3> position = Vec<3>(ch_pos_x, 0, ch_pos_z)
                + Vec<3>(foliage.x, foliage.y, foliage.z);
            spawners.push_back((*f_info.particle_spawner)(position, this->rng));
        }
        return spawners;
    }

    Terrain::LoadedChunk Terrain::load_chunk(
        i64 chunk_x, i64 chunk_z, 
        Interactables* interactables, engine::Window& window,
        const std::shared_ptr<World>& world,
        bool in_bounds
    ) {
        if(!in_bounds) {
            return {
                chunk_x, chunk_z, false,
                engine::Mesh(Renderer::mesh_attribs),
                this->build_chunk_water_geometry(chunk_x, chunk_z),
                std::unordered_map<Foliage::Type, std::vector<Mat<4>>>(),
                std::unordered_map<Building::Type, std::vector<Mat<4>>>(),
                std::unordered_map<Resource::Type, std::vector<Mat<4>>>(),
                std::unordered_map<TrackPiece::Type, std::vector<Mat<4>>>(),
                std::vector<std::shared_ptr<Interactable>>(),
                std::vector<ParticleSpawner>()
            };
        }
        return {
            chunk_x, chunk_z, false, 
            this->build_chunk_terrain_geometry((u64) chunk_x, (u64) chunk_z),
            this->build_chunk_water_geometry(chunk_x, chunk_z),
            this->collect_foliage_transforms((u64) chunk_x, (u64) chunk_z),
            this->collect_building_transforms((u64) chunk_x, (u64) chunk_z),
            this->collect_resource_transforms((u64) chunk_x, (u64) chunk_z),
            this->collect_track_piece_transforms((u64) chunk_x, (u64) chunk_z),
            this->create_chunk_interactables(
                (u64) chunk_x, (u64) chunk_z, interactables, window, world
            ),
            this->create_chunk_particle_spawners((u64) chunk_x, (u64) chunk_z)
        };
    }

    bool Terrain::chunk_in_draw_distance(
        u64 chunk_x, u64 chunk_z, u64 draw_distance
    ) const {
        i64 diff_x = (i64) chunk_x - this->view_chunk_x;
        i64 diff_z = (i64) chunk_z - this->view_chunk_z;
        i64 mh_dist = std::max(llabs(diff_x), llabs(diff_z));
        return mh_dist <= (i64) draw_distance;
    }

    bool Terrain::chunk_loaded(u64 chunk_x, u64 chunk_z, size_t& index) const {
        for(size_t chunk_i = 0; chunk_i < this->loaded_chunks.size(); chunk_i += 1) {
            const LoadedChunk& chunk = this->loaded_chunks.at(chunk_i);
            if(chunk.x == (i64) chunk_x && chunk.z == (i64) chunk_z) {
                index = chunk_i;
                return true;
            }
        }
        return false;
    }

    void Terrain::load_chunks_around(
        const Vec<3>& position, u64 draw_distance, Interactables* interactables,
        engine::Window& window, const std::shared_ptr<World>& world
    ) {
        this->view_chunk_x = (u64) (position.x() / this->tile_size / this->chunk_tiles);
        this->view_chunk_z = (u64) (position.z() / this->tile_size / this->chunk_tiles);
        // despawn chunks that are too far away
        for(size_t chunk_i = 0; chunk_i < this->loaded_chunks.size();) {
            const LoadedChunk& chunk = this->loaded_chunks.at(chunk_i);
            if(this->chunk_in_draw_distance(chunk.x, chunk.z, draw_distance)) {
                chunk_i += 1;
                continue;
            }
            this->loaded_chunks.erase(this->loaded_chunks.begin() + chunk_i);
        }
        // spawn and update chunks in the draw distance
        i64 viewed_start_x = this->view_chunk_x - (i64) draw_distance;
        i64 viewed_end_x = this->view_chunk_x + (i64) draw_distance;
        i64 viewed_start_z = this->view_chunk_z - (i64) draw_distance;
        i64 viewed_end_z = this->view_chunk_z + (i64) draw_distance;
        for(i64 chunk_x = viewed_start_x; chunk_x <= viewed_end_x; chunk_x += 1) {
            for(i64 chunk_z = viewed_start_z; chunk_z <= viewed_end_z; chunk_z += 1) {
                bool in_bounds = chunk_x >= 0 && chunk_z >= 0
                    && chunk_x < (i64) this->width_chunks
                    && chunk_z < (i64) this->height_chunks;
                size_t chunk_i;
                if(!this->chunk_loaded(chunk_x, chunk_z, chunk_i)) {
                    this->loaded_chunks.push_back(
                        this->load_chunk(
                            chunk_x, chunk_z, interactables, window, world,
                            in_bounds
                        )
                    );
                    continue; 
                }
                LoadedChunk& chunk = this->loaded_chunks.at(chunk_i);
                if(chunk.modified) {
                    chunk = this->load_chunk(
                        chunk_x, chunk_z, interactables, window, world,
                        in_bounds
                    );
                }
            }
        }
    }


    const Building* Terrain::building_at(
        i64 tile_x, i64 tile_z, u64* chunk_x_out, u64* chunk_z_out
    ) const {
        i64 r_chunk_x = tile_x / (i64) this->chunk_tiles;
        i64 r_chunk_z = tile_z / (i64) this->chunk_tiles;
        for(i64 chunk_x = r_chunk_x - 1; chunk_x <= r_chunk_x + 1; chunk_x += 1) {
            if(chunk_x < 0 || (u64) chunk_x >= this->width_chunks) { continue; }
            for(i64 chunk_z = r_chunk_z - 1; chunk_z <= r_chunk_z + 1; chunk_z += 1) {
                if(chunk_z < 0 || (u64) chunk_z >= this->height_chunks) { continue; }
                const ChunkData& chunk = this->chunk_at(chunk_x, chunk_z);
                i64 chunk_offset_x = chunk_x * this->chunk_tiles;
                i64 chunk_offset_z = chunk_z * this->chunk_tiles;
                for(const Building& building: chunk.buildings) {
                    const Building::TypeInfo& type 
                        = Building::types().at((size_t) building.type);
                    i64 start_x = chunk_offset_x + (i64) building.x;
                    i64 end_x = start_x + (i64) type.width;
                    i64 start_z = chunk_offset_z + (i64) building.z;
                    i64 end_z = start_z + (i64) type.height;
                    bool overlaps = start_x <= tile_x && tile_x < end_x
                        && start_z <= tile_z && tile_z < end_z;
                    if(!overlaps) { continue; }
                    if(chunk_x_out != nullptr) { *chunk_x_out = (u64) chunk_x; }
                    if(chunk_z_out != nullptr) { *chunk_z_out = (u64) chunk_z; }
                    return &building;
                }
            }
        }
        return nullptr;
    }

    Building* Terrain::building_at(
        i64 tile_x, i64 tile_z, u64* chunk_x_out, u64* chunk_z_out
    ) {
        return (Building*) ((const Terrain*) this)->building_at(
            tile_x, tile_z, chunk_x_out, chunk_z_out
        );
    }

    const Bridge* Terrain::bridge_at(
        i64 tile_x_s, i64 tile_z_s, f64 closest_to_height
    ) const {
        if(tile_x_s < 0 || tile_z_s < 0) { return nullptr; }
        u64 tile_x = (u64) tile_x_s;
        u64 tile_z = (u64) tile_z_s;
        const Bridge* closest = nullptr;
        f64 clostest_height_diff = INFINITY;
        for(const Bridge& bridge: this->bridges) {
            if(tile_x < bridge.start_x || bridge.end_x < tile_x) { continue; }
            if(tile_z < bridge.start_z || bridge.end_z < tile_z) { continue; }
            f64 height_diff = fabs((f64) bridge.floor_y - closest_to_height);
            if(height_diff > clostest_height_diff) { continue; }
            closest = &bridge;
            clostest_height_diff = height_diff;
        }
        return closest;
    }

    const Resource* Terrain::resource_at(i64 tile_x_s, i64 tile_z_s) const {
        if(tile_x_s < 0 || tile_z_s < 0) { return nullptr; }
        u64 tile_x = (u64) tile_x_s;
        u64 tile_z = (u64) tile_z_s;
        if(tile_x >= this->width || tile_z >= this->height) { return nullptr; }
        u64 chunk_x = tile_x / this->tiles_per_chunk();
        u64 chunk_z = tile_z / this->tiles_per_chunk();
        u64 rel_x = tile_x % this->tiles_per_chunk();
        u64 rel_z = tile_z % this->tiles_per_chunk();
        const ChunkData& chunk = this->chunk_at(chunk_x, chunk_z);
        for(const Resource& resource: chunk.resources) {
            if(resource.x != rel_x || resource.z != rel_z) { continue; }
            return &resource;
        }
        return nullptr;
    }

    u64 Terrain::track_pieces_at(
        i64 tile_x_s, i64 tile_z_s, 
        std::vector<TrackPiece*>* collected_out
    ) {
        if(tile_x_s < 0 || tile_z_s < 0) { return 0; }
        u64 tile_x = (u64) tile_x_s;
        u64 tile_z = (u64) tile_z_s;
        if(tile_x >= this->width || tile_z >= this->height) { return 0; }
        u64 chunk_x = tile_x / this->tiles_per_chunk();
        u64 chunk_z = tile_z / this->tiles_per_chunk();
        u64 rel_x = tile_x % this->tiles_per_chunk();
        u64 rel_z = tile_z % this->tiles_per_chunk();
        ChunkData& chunk = this->chunk_at(chunk_x, chunk_z);
        u64 collected_count = 0;
        for(TrackPiece& track_piece: chunk.track_pieces) {
            if(track_piece.x != rel_x || track_piece.z != rel_z) { continue; }
            collected_count += 1;
            if(collected_out == nullptr) { continue; }
            collected_out->push_back(&track_piece);
        }
        return collected_count;
    }

    static const i64 collision_test_dist = 1;

    bool Terrain::valid_player_position(
        const AbsCollider& player_collider, bool water_is_obstacle
    ) const {
        if(water_is_obstacle) {
            f64 min_elev = std::min(
                this->elevation_at(player_collider.start),
                this->elevation_at(player_collider.end)
            );
            if(min_elev + 0.5 <= water_height) { return false; }
        }
        for(const Bridge& bridge: this->bridges) {
            bridge.report_malformed();
            // "move" the start and end tiles of the bridge 
            // to the start and ends of the player collider
            // this minimizes the number of tiles we need to check
            u64 x = std::max(
                (u64) (player_collider.start.x() / this->tile_size),
                bridge.start_x
            );
            u64 z = std::max(
                (u64) (player_collider.start.z() / this->tile_size),
                bridge.start_z
            );
            u64 end_x = std::min(
                (u64) (player_collider.end.x() / this->tile_size),
                bridge.end_x
            );
            u64 end_z = std::min(
                (u64) (player_collider.end.z() / this->tile_size),
                bridge.end_z
            );
            if(end_x < x || end_z < z) { continue; }
            for(;;) {
                for(const RelCollider& collider: bridge.colliders()) {
                    Vec<3> segment_pos = Vec<3>(x, 0, z) * this->tile_size
                        + Vec<3>(2.5, (f64) bridge.floor_y, 2.5);
                    bool collides = player_collider
                        .collides_with(collider.at(segment_pos));
                    if(collides) { return false; }
                }
                if(x < end_x) { x += 1; }
                else if(z < end_z) { z += 1; }
                else { break; }
            }
        }
        u64 start_x = (u64) std::max(this->view_chunk_x - collision_test_dist, (i64) 0);
        u64 end_x = std::min((u64) (this->view_chunk_x + collision_test_dist), this->width_chunks - 1);
        u64 start_z = (u64) std::max(this->view_chunk_z - collision_test_dist, (i64) 0);
        u64 end_z = std::min((u64) (this->view_chunk_z + collision_test_dist), this->height_chunks - 1);
        for(u64 chunk_x = start_x; chunk_x <= end_x; chunk_x += 1) {
            for(u64 chunk_z = start_z; chunk_z <= end_z; chunk_z += 1) {
                const ChunkData& chunk = this->chunk_at(chunk_x, chunk_z);
                u64 chunk_tile_x = chunk_x * this->tiles_per_chunk();
                u64 chunk_tile_z = chunk_z * this->tiles_per_chunk();
                for(const Building& building: chunk.buildings) {
                    const Building::TypeInfo& type = building.get_type_info();
                    u64 tile_x = chunk_tile_x + building.x;
                    u64 tile_z = chunk_tile_z + building.z;
                    Vec<3> offset_tiles = Vec<3>(tile_x, 0, tile_z)
                        + Vec<3>(type.width / 2.0, 0, type.height / 2.0);
                    Vec<3> offset = offset_tiles * this->units_per_tile()
                        + Vec<3>(0, this->elevation_at(tile_x, tile_z), 0);
                    for(const RelCollider& collider: type.colliders) {
                        bool collision = collider
                            .at(offset)
                            .collides_with(player_collider);
                        if(collision) { return false; }
                    }
                }
                for(const Foliage& foliage: chunk.foliage) {
                    const Foliage::TypeInfo& type = foliage.get_type_info();
                    u64 x = chunk_tile_x * this->units_per_tile() + foliage.x;
                    u64 z = chunk_tile_z * this->units_per_tile() + foliage.z;
                    i64 h = this->elevation_at(Vec<3>(x, 0, z));
                    bool collision = type.collider
                        .at(Vec<3>(x, h, z))
                        .collides_with(player_collider);
                    if(collision) { return false; }
                }
            }
        }
        return true;
    }

    void Terrain::remove_foliage_at(i64 tile_x, i64 tile_z) {
        if(tile_x < 0 || (u64) tile_x >= this->width) { return; }
        if(tile_z < 0 || (u64) tile_z >= this->width) { return; }
        u64 chunk_x = (u64) tile_x / this->chunk_tiles;
        u64 chunk_z = (u64) tile_z / this->chunk_tiles;
        ChunkData& chunk = this->chunk_at(chunk_x, chunk_z);
        for(size_t foliage_i = 0; foliage_i < chunk.foliage.size();) {
            const Foliage& foliage = chunk.foliage.at(foliage_i);
            u64 foliage_tile_x = chunk_x * this->chunk_tiles
                + (u64) foliage.x / this->tile_size;
            u64 foliage_tile_z = chunk_z * this->chunk_tiles
                + (u64) foliage.z / this->tile_size;
            if(foliage_tile_x != (u64) tile_x || foliage_tile_z != (u64) tile_z) {
                foliage_i += 1;
                continue;
            }
            chunk.foliage.erase(chunk.foliage.begin() + foliage_i);
        }
        this->reload_chunk_at(chunk_x, chunk_z);
    }

    void Terrain::adjust_area_foliage(
        i64 min_x, i64 min_z, i64 max_x, i64 max_z
    ) {
        u64 c_min_x = (u64) std::max(min_x, (i64) 0);
        u64 c_min_z = (u64) std::max(min_z, (i64) 0);
        u64 c_max_x = std::min((u64) max_x, this->width_in_tiles() - 1);
        u64 c_max_z = std::min((u64) max_z, this->height_in_tiles() - 1);
        u64 min_ch_x = c_min_x / this->tiles_per_chunk();
        u64 min_ch_z = c_min_z / this->tiles_per_chunk();
        u64 max_ch_x = c_max_x / this->tiles_per_chunk();
        u64 max_ch_z = c_max_z / this->tiles_per_chunk();
        for(u64 ch_x = min_ch_x; ch_x <= max_ch_x; ch_x += 1) {
            for(u64 ch_z = min_ch_z; ch_z <= max_ch_z; ch_z += 1) {
                Terrain::ChunkData& chunk = this->chunk_at(ch_x, ch_z);
                for(size_t fol_i = 0; fol_i < chunk.foliage.size();) {
                    Foliage& fol = chunk.foliage[fol_i];
                    Vec<3> fol_pos = Vec<3>(ch_x, 0, ch_z) 
                        * this->tiles_per_chunk() * this->units_per_tile()
                        + Vec<3>(fol.x, 0, fol.z);
                    fol.y = this->elevation_at(fol_pos);
                    if(fol.y < 0.0) {
                        chunk.foliage.erase(chunk.foliage.begin() + fol_i);
                    } else {
                        fol_i += 1;
                    }
                }
                this->reload_chunk_at(ch_x, ch_z);
            }
        }
    }

    f64 Terrain::average_area_elevation(u64 s_x, u64 s_z, u64 w, u64 h) const {
        u64 e_x = std::min(s_x + w, this->width_in_tiles());
        u64 e_z = std::min(s_z + h, this->width_in_tiles());
        f64 sum = 0;
        for(u64 x = s_x; x <= e_x; x += 1) {
            for(u64 z = s_z; z <= e_z; z += 1) {
                sum += (f64) this->elevation_at(x, z);
            }
        }
        u64 node_count = (e_x - s_x + 1) * (e_z - s_z + 1);
        return sum / (f64) node_count;
    }

    bool Terrain::vert_area_elev_mutable(
        u64 min_x, u64 min_z, u64 max_x, u64 max_z,
        std::function<bool (i16)> modified 
    ) const {
        // ensure that the ground beneath all buildings and train tracks
        // in a 1 chunk radius don't get modified 
        // (works as long as no building is larger than a chunk) 
        u64 start_ch_x = min_x / this->tiles_per_chunk();
        if(start_ch_x >= 1) { start_ch_x -= 1; }
        u64 start_ch_z = min_z / this->tiles_per_chunk();
        if(start_ch_z >= 1) { start_ch_z -= 1; }
        u64 end_ch_x = (max_x / this->tiles_per_chunk()) + 1;
        end_ch_x = std::min(end_ch_x, this->width_in_chunks() - 1);
        u64 end_ch_z = (max_z / this->tiles_per_chunk()) + 1;
        end_ch_z = std::min(end_ch_z, this->height_in_chunks() - 1);
        for(u64 ch_x = start_ch_x; ch_x <= end_ch_x; ch_x += 1) {
            for(u64 ch_z = start_ch_z; ch_z <= end_ch_z; ch_z += 1) {
                const Terrain::ChunkData& chunk = this->chunk_at(ch_x, ch_z);
                for(const Building& building: chunk.buildings) {
                    const Building::TypeInfo& info = building.get_type_info();
                    // determine the vertex bounds of the building
                    u64 b_start_x = ch_x * this->tiles_per_chunk() + building.x;
                    u64 b_start_z = ch_z * this->tiles_per_chunk() + building.z;
                    u64 b_end_x = b_start_x + info.width;
                    u64 b_end_z = b_start_z + info.height;
                    // determine the intersection
                    bool has_zero_overlap = min_x > b_end_x || min_z > b_end_z
                        || max_x < b_start_x || max_z < b_start_z;
                    if(has_zero_overlap) { continue; }
                    u64 i_start_x = std::max(min_x, b_start_x);
                    u64 i_start_z = std::max(min_z, b_start_z);
                    u64 i_end_x = std::min(max_x, b_end_x);
                    u64 i_end_z = std::min(max_z, b_end_z);
                    // ensure that each point in the intersection does not get
                    // modified
                    bool any_modified = false;
                    for(u64 i_x = i_start_x; i_x <= i_end_x; i_x += 1) {
                        for(u64 i_z = i_start_z; i_z <= i_end_z; i_z += 1) {
                            i16 elev = this->elevation_at(i_x, i_z);
                            any_modified |= modified(elev);
                            if(any_modified) { break; }
                        }
                        if(any_modified) { break; }
                    }
                    if(any_modified) { return false; }
                }
                for(const TrackPiece& tp: chunk.track_pieces) {
                    u64 tp_x = ch_x * this->tiles_per_chunk() + tp.x;
                    u64 tp_z = ch_z * this->tiles_per_chunk() + tp.z;
                    // determine the intersection
                    bool has_zero_overlap = max_x < tp_x || max_z < tp_z
                        || min_x > (tp_x + 1) || min_z > (tp_z + 1);
                    if(has_zero_overlap) { continue; }
                    u64 i_start_x = std::max(min_x, tp_x);
                    u64 i_start_z = std::max(min_z, tp_z);
                    u64 i_end_x = std::min(max_x, tp_x + 1);
                    u64 i_end_z = std::min(max_z, tp_z + 1);
                    // ensure that each point in the intersection does not get
                    // modified
                    bool any_modified = false;
                    for(u64 i_x = i_start_x; i_x <= i_end_x; i_x += 1) {
                        for(u64 i_z = i_start_z; i_z <= i_end_z; i_z += 1) {
                            i16 elev = this->elevation_at(i_x, i_z);
                            any_modified |= modified(elev);
                            if(any_modified) { break; }
                        }
                        if(any_modified) { break; }
                    }
                    if(any_modified) { return false; }
                }
            }
        }
        // check the ground beneath all bridges
        for(const Bridge& bridge: this->bridges) {
            u64 b_end_x = bridge.end_x + 1;
            u64 b_end_z = bridge.end_z + 1;
            // determine the intersection
            bool has_zero_overlap = min_x > b_end_x || min_z > b_end_z
                || max_x < bridge.start_x || max_z < bridge.start_z;
            if(has_zero_overlap) { continue; }
            u64 i_start_x = std::max(min_x, bridge.start_x);
            u64 i_start_z = std::max(min_z, bridge.start_z);
            u64 i_end_x = std::min(max_x, b_end_x);
            u64 i_end_z = std::min(max_z, b_end_z);
            // ensure that each point in the intersection does not get
            // modified
            bool any_modified = false;
            for(u64 i_x = i_start_x; i_x <= i_end_x; i_x += 1) {
                for(u64 i_z = i_start_z; i_z <= i_end_z; i_z += 1) {
                    i16 elev = this->elevation_at(i_x, i_z);
                    any_modified |= modified(elev);
                    if(any_modified) { break; }
                }
                if(any_modified) { break; }
            }
            if(any_modified) { return false; }
        }
        return true;
    }

    bool Terrain::vert_area_above_water(
        u64 min_x, u64 min_z, u64 max_x, u64 max_z
    ) const {
        for(u64 x = min_x; x <= max_x; x += 1) {
            for(u64 z = min_z; z <= max_z; z += 1) {
                if(this->elevation_at(x, z) < 0.0) { return false; }
            }
        }
        return true;
    }

    void Terrain::set_area_elevation(
        u64 min_x, u64 min_z, u64 max_x, u64 max_z, i16 elev
    ) {
        for(u64 x = min_x; x <= max_x; x += 1) {
            for(u64 z = min_z; z <= max_z; z += 1) {
                this->elevation_at(x, z) = elev;
            }
        }
    }

    void Terrain::place_building(
        Building::Type type, u64 tile_x, u64 tile_z, 
        std::optional<ComplexId> complex,
        std::optional<i16> elevation
    ) {
        const Building::TypeInfo& type_info = Building::types()
            .at((size_t) type);
        i16 written_elev;
        if(elevation.has_value()) { 
            written_elev = *elevation;
        } else {
            written_elev = (i16) this->average_area_elevation(
                tile_x, tile_z, type_info.width, type_info.height
            );
        }
        this->set_area_elevation(
            tile_x, tile_z, tile_x + type_info.width, tile_z + type_info.height, 
            written_elev
        );
        for(u64 x = tile_x; x < tile_x + type_info.width; x += 1) {
            for(u64 z = tile_z; z < tile_z + type_info.height; z += 1) {
                this->remove_foliage_at((i64) x, (i64) z);    
            }
        }
        this->adjust_area_foliage(
            (i64) tile_x - 1, (i64) tile_z - 1, 
            (i64) (tile_x + type_info.width + 1), 
            (i64) (tile_z + type_info.height + 1)
        );
        u64 chunk_x = tile_x / this->tiles_per_chunk();
        u64 chunk_z = tile_z / this->tiles_per_chunk();
        Terrain::ChunkData& chunk = this->chunk_at(chunk_x, chunk_z);
        u64 rel_x = tile_x % this->tiles_per_chunk();
        u64 rel_z = tile_z % this->tiles_per_chunk();
        chunk.buildings.push_back((Building) {
            type, (u8) rel_x, (u8) rel_z, complex
        });
    }
    

    static const i64 tile_selection_range_chunks = 2;

    std::pair<u64, u64> Terrain::find_selected_terrain_tile(
        Vec<2> cursor_pos_ndc, const Renderer& renderer, Vec<3> tile_offset
    ) const {
        i64 start_x = (this->viewed_chunk_x() - tile_selection_range_chunks) 
            * (i64) this->tiles_per_chunk();
        i64 end_x = (this->viewed_chunk_x() + tile_selection_range_chunks) 
            * (i64) this->tiles_per_chunk();
        i64 start_z = (this->viewed_chunk_z() - tile_selection_range_chunks) 
            * (i64) this->tiles_per_chunk();
        i64 end_z = (this->viewed_chunk_z() + tile_selection_range_chunks) 
            * (i64) this->tiles_per_chunk();
        std::pair<u64, u64> current;
        f64 current_dist = INFINITY;
        for(
            i64 tile_x = std::max(start_x, (i64) 0); 
            tile_x < std::min(end_x, (i64) this->width_in_tiles());
            tile_x += 1
        ) {
            for(
                i64 tile_z = std::max(start_z, (i64) 0); 
                tile_z < std::min(end_z, (i64) this->width_in_tiles());
                tile_z += 1
            ) {
                Vec<3> pos = (Vec<3>(tile_x, 0, tile_z) + tile_offset)
                    * this->units_per_tile();
                pos.y() = this->elevation_at(
                    (u64) tile_x, (u64) tile_z);
                Vec<2> pos_ndc = renderer.world_to_ndc(pos);
                f64 dist = (pos_ndc - cursor_pos_ndc).len();
                if(dist > current_dist) { continue; }
                current = { (u64) tile_x, (u64) tile_z };
                current_dist = dist;
            }
        }
        return current;
    }

    i64 Terrain::compute_unemployment() const {
        i64 unemployment = 0;
        for(const ChunkData& chunk: this->chunks) {
            for(const Building& building: chunk.buildings) {
                const Building::TypeInfo& type = building.get_type_info();
                unemployment += (i64) type.residents - (i64) type.workers;
            }
        }
        return unemployment;
    }


    void Terrain::spawn_particles(
        const engine::Window& window, ParticleManager& particles
    ) {
        for(LoadedChunk& chunk: this->loaded_chunks) {
            for(ParticleSpawner& spawner: chunk.particle_spawners) {
                spawner.spawn(window, particles);
            }
        }
    }


    void Terrain::render_loaded_chunks(
        engine::Scene& scene, Renderer& renderer,
        const engine::Window& window
    ) {
        std::unordered_map<Foliage::Type, std::vector<Mat<4>>> foliage_instances;
        std::unordered_map<Building::Type, std::vector<Mat<4>>> building_instances;
        std::unordered_map<Resource::Type, std::vector<Mat<4>>> resource_instances;
        std::unordered_map<TrackPiece::Type, std::vector<Mat<4>>> track_piece_instances;
        for(LoadedChunk& chunk: this->loaded_chunks) {
            Vec<3> chunk_offset = Vec<3>(chunk.x, 0, chunk.z)
                * this->chunk_tiles * this->tile_size;
            this->render_chunk_ground(
                chunk, scene.get(Terrain::ground_texture), chunk_offset, renderer
            );
            for(const auto& [foliage_type, instances]: chunk.foliage) {
                std::vector<Mat<4>>& f_inst = foliage_instances[foliage_type];
                f_inst.insert(f_inst.end(), instances.begin(), instances.end());
            }
            for(const auto& [building_type, instances]: chunk.buildings) {
                std::vector<Mat<4>>& b_inst = building_instances[building_type];
                b_inst.insert(b_inst.end(), instances.begin(), instances.end());
            }
            for(const auto& [resource_type, instances]: chunk.resources) {
                std::vector<Mat<4>>& r_inst = resource_instances[resource_type];
                r_inst.insert(r_inst.end(), instances.begin(), instances.end());
            }
            for(const auto& [track_piece_type, instances]: chunk.track_pieces) {
                std::vector<Mat<4>>& p_inst = track_piece_instances[track_piece_type];
                p_inst.insert(p_inst.end(), instances.begin(), instances.end());
            }
        }
        for(const auto& [foliage_type, instances]: foliage_instances) {
            engine::Model& model = scene.get(
                Foliage::types().at((size_t) foliage_type).model
            );
            renderer.render(model, instances);
        }
        for(const auto& [building_type, instances]: building_instances) {
            const Building::TypeInfo& type_info
                = Building::types().at((size_t) building_type);
            type_info.render_buildings(window, scene, renderer, instances);
        }
        for(const auto& [resource_type, instances]: resource_instances) {
            const Resource::TypeInfo& type_info
                = Resource::types().at((size_t) resource_type);
            renderer.render(
                scene.get(type_info.model), instances, nullptr, 0.0,
                engine::FaceCulling::Enabled,
                engine::DepthTesting::Enabled,
                &scene.get(type_info.texture)
            );
        }
        for(const auto& [track_piece_type, instances]: track_piece_instances) {
            const TrackPiece::TypeInfo& piece_type_info = TrackPiece::types()
                .at((size_t) track_piece_type);
            renderer.render(
                scene.get(piece_type_info.model), instances, nullptr, 0.0
            );
        }
        this->render_bridges(scene, renderer);
    }

    void Terrain::render_chunk_ground(
        LoadedChunk& loaded_chunk,
        const engine::Texture& ground_texture, 
        const Vec<3>& chunk_offset,
        Renderer& renderer
    ) {
        renderer.render(
            loaded_chunk.terrain, ground_texture, 
            Mat<4>(),
            std::array { Mat<4>::translate(chunk_offset) }
        );
    }

    void Terrain::render_bridges(
        engine::Scene& scene, Renderer& renderer
    ) {
        i64 view_start_x = (this->viewed_chunk_x() - tile_selection_range_chunks) 
            * (i64) this->tiles_per_chunk();
        i64 view_end_x = (this->viewed_chunk_x() + tile_selection_range_chunks) 
            * (i64) this->tiles_per_chunk();
        i64 view_start_z = (this->viewed_chunk_z() - tile_selection_range_chunks) 
            * (i64) this->tiles_per_chunk();
        i64 view_end_z = (this->viewed_chunk_z() + tile_selection_range_chunks) 
            * (i64) this->tiles_per_chunk();
        std::vector<std::vector<Mat<4>>> instances;
        instances.resize(Bridge::types().size());
        for(const Bridge& bridge: this->bridges) {
            bool not_visible = (i64) bridge.end_x < view_start_x
                || (i64) bridge.start_x > view_end_x
                || (i64) bridge.end_z < view_start_z
                || (i64) bridge.start_z > view_end_z;
            if(not_visible) { continue; }
            size_t type_id = (size_t) bridge.type;
            std::vector<Mat<4>> bridge_instances = bridge
                .get_instances(this->units_per_tile());
            instances[type_id].insert(
                instances[type_id].end(), 
                bridge_instances.begin(), bridge_instances.end()
            );
        }
        for(size_t type_id = 0; type_id < instances.size(); type_id += 1) {
            if(instances[type_id].size() == 0) { continue; }
            const Bridge::TypeInfo& type = Bridge::types().at(type_id);
            renderer.render(scene.get(type.model), instances[type_id]);
        }
    }

    static Vec<4> water_light_color = Vec<4>(245, 237, 186, 255) / 255;
    static Vec<4> water_base_color = Vec<4>(126, 196, 193, 255) / 255;
    static Vec<4> water_dark_color = Vec<4>(52, 133, 157, 255) / 255;

    void Terrain::render_water(
        engine::Scene& scene, Renderer& renderer, 
        const engine::Window& window
    ) {
        const engine::Texture& normal_map = scene.get(Terrain::water_texture);
        engine::Shader& shader = scene.get(Terrain::water_shader);
        shader.set_uniform("u_view_projection", renderer.compute_view_proj());
        shader.set_uniform("u_light_color", water_light_color);
        shader.set_uniform("u_base_color", water_base_color);
        shader.set_uniform("u_dark_color", water_dark_color);
        shader.set_uniform("u_nmap", normal_map);
        // normal map is assumed to be square
        assert(normal_map.width() == normal_map.height());
        f64 nmap_scale = 16.0 / normal_map.width(); // 16px / unit
        shader.set_uniform("u_nmap_scale", nmap_scale);
        shader.set_uniform("u_time", window.time());
        renderer.set_fog_uniforms(shader);
        renderer.set_shadow_uniforms(shader);
        for(LoadedChunk& chunk: this->loaded_chunks) {
            Vec<3> chunk_offset = Vec<3>(chunk.x, 0, chunk.z)
                * this->chunk_tiles * this->tile_size;
            shader.set_uniform("u_local_transf", Mat<4>());
            shader.set_uniform("u_model_transf", Mat<4>::translate(chunk_offset));
            chunk.water.render(shader, renderer.output().as_target());
        }
    }


    Terrain::Terrain(
        const Serialized& serialized, u64 tile_size, u64 chunk_tiles,
        const engine::Arena& buffer
    ) {
        this->width = serialized.width;
        this->height = serialized.height;
        this->tile_size = tile_size;
        this->chunk_tiles = chunk_tiles;
        this->view_chunk_x = 0;
        this->view_chunk_z = 0;
        buffer.copy_into(serialized.elevation, this->elevation);
        this->width_chunks = (u64) ceil((f64) width / chunk_tiles);
        this->height_chunks = (u64) ceil((f64) height / chunk_tiles);
        buffer.copy_into<ChunkData::Serialized, ChunkData>(
            serialized.chunks, this->chunks,
            [&](const auto& c) {
                return ChunkData(this->chunk_tiles, c, buffer); 
            }
        );
        buffer.copy_into(serialized.bridges, this->bridges);
    }

    Terrain::ChunkData::ChunkData(
        u64 size_tiles,
        const ChunkData::Serialized& serialized, const engine::Arena& buffer
    ) {
        this->size_tiles = size_tiles;
        buffer.copy_into(serialized.foliage, this->foliage);
        buffer.copy_into(serialized.buildings, this->buildings);
        buffer.copy_into(serialized.resources, this->resources);
        buffer.copy_into(serialized.track_pieces, this->track_pieces);
        buffer.copy_into(serialized.paths, this->paths);
    }

    Terrain::ChunkData::Serialized Terrain::ChunkData::serialize(
        engine::Arena& buffer
    ) const {
        return {
            buffer.alloc(this->foliage),
            buffer.alloc(this->buildings),
            buffer.alloc(this->resources),
            buffer.alloc(this->track_pieces),
            buffer.alloc(this->paths)
        };
    }

    Terrain::Serialized Terrain::serialize(engine::Arena& buffer) const {
        return {
            this->width, this->height,
            buffer.alloc(this->elevation),
            buffer.alloc<ChunkData, ChunkData::Serialized>(
                this->chunks, 
                [&](const auto& c) { return c.serialize(buffer); }
            ),
            buffer.alloc(this->bridges)
        };
    }

}