
#include "terrain.hpp"

namespace houseofatmos::outside {

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


    void Terrain::generate_elevation(u32 seed) {
        for(u64 x = 0; x <= this->width; x += 1) {
            for(u64 z = 0; z <= this->height; z += 1) {
                f64 height = 0.0;
                height += perlin_noise(seed, Vec<2>(x / 1.0, z / 1.0));
                height += perlin_noise(seed, Vec<2>((f64) x / 40.0, (f64) z / 40.0)) * 10
                    * (perlin_noise(seed, Vec<2>(x / 128.0, z / 128.0)) * 5);
                this->elevation_at(x, z) = (i16) height;
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
                f64 slope_factor = 1 - (height_diff / stone_min_height_diff);
                ChunkData& chunk = this->chunk_at(chunk_x, chunk_z);
                f64 noise_val = perlin_noise(
                    seed, Vec<2>(tile_x / 25.0, tile_z / 25.0)
                ) * 0.5 + 0.5; // normalize to 0..1
                for(size_t type_i = 0; type_i < Foliage::types.size(); type_i += 1) {
                    const Foliage::TypeInfo& type = Foliage::types.at(type_i);
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


    void Terrain::build_water_plane() {
        engine::Mesh& p = this->water_plane;
        // tl---tr
        //  | \ |
        // bl---br
        p.start_vertex();
            p.put_f32({ -0.5, water_height,  0.5 });
            p.put_f32({ 0, 1 });
        u16 tl = p.complete_vertex();
        p.start_vertex();
            p.put_f32({  0.5, water_height,  0.5 });
            p.put_f32({ 1, 1 });
        u16 tr = p.complete_vertex();
        p.start_vertex();
            p.put_f32({ -0.5, water_height, -0.5 });
            p.put_f32({ 0, 0 });
        u16 bl = p.complete_vertex();
        p.start_vertex();
            p.put_f32({  0.5, water_height, -0.5 });
            p.put_f32({ 1, 0 });
        u16 br = p.complete_vertex();
        p.add_element(tl, bl, br);
        p.add_element(tl, br, tr);
        p.submit();
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

    engine::Mesh Terrain::build_chunk_geometry(u64 chunk_x, u64 chunk_z) const {
        auto geometry = engine::Mesh(Renderer::mesh_attribs);
        const ChunkData& chunk_data = this->chunk_at(chunk_x, chunk_z);
        u64 start_x = chunk_x * this->chunk_tiles;
        u64 end_x = std::min((chunk_x + 1) * this->chunk_tiles, this->width);
        u64 start_z = chunk_z * this->chunk_tiles;
        u64 end_z = std::min((chunk_z + 1) * this->chunk_tiles, this->height);
        for(u64 x = start_x; x < end_x; x += 1) {
            for(u64 z = start_z; z < end_z; z += 1) {
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
                bool is_path = chunk_data.path_at(left, top);
                Vec<2> uv_bl = { 0.00, 0.00 };
                Vec<2> uv_br = { 0.49, 0.00 };
                Vec<2> uv_tl = { 0.00, 0.49 };
                Vec<2> uv_tr = { 0.49, 0.49 };
                if(tr_bl_height_diff > tl_br_height_diff) {
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

    Terrain::LoadedChunk Terrain::load_chunk(u64 chunk_x, u64 chunk_z) const {
        return {
            chunk_x, chunk_z, false, 
            build_chunk_geometry(chunk_x, chunk_z),
            collect_foliage_transforms(chunk_x, chunk_z),
            collect_building_transforms(chunk_x, chunk_z)
        };
    }

    bool Terrain::chunk_in_draw_distance(u64 chunk_x, u64 chunk_z) const {
        i64 diff_x = (i64) chunk_x - this->view_chunk_x;
        i64 diff_z = (i64) chunk_z - this->view_chunk_z;
        i64 mh_dist = llabs(diff_x) + llabs(diff_z);
        return mh_dist <= this->draw_distance;
    }

    bool Terrain::chunk_loaded(u64 chunk_x, u64 chunk_z, size_t& index) const {
        for(size_t chunk_i = 0; chunk_i < this->loaded_chunks.size(); chunk_i += 1) {
            const LoadedChunk& chunk = this->loaded_chunks.at(chunk_i);
            if(chunk.x == chunk_x && chunk.z == chunk_z) {
                index = chunk_i;
                return true;
            }
        }
        return false;
    }

    void Terrain::load_chunks_around(const Vec<3>& position) {
        this->view_chunk_x = (u64) (position.x() / this->tile_size / this->chunk_tiles);
        this->view_chunk_z = (u64) (position.z() / this->tile_size / this->chunk_tiles);
        // despawn chunks that are too far away
        for(size_t chunk_i = 0; chunk_i < this->loaded_chunks.size();) {
            const LoadedChunk& chunk = this->loaded_chunks.at(chunk_i);
            if(this->chunk_in_draw_distance(chunk.x, chunk.z)) {
                chunk_i += 1;
                continue;
            }
            this->loaded_chunks.erase(this->loaded_chunks.begin() + chunk_i);
        }
        // spawn and update chunks in the draw distance
        i64 viewed_start_x = std::max(
            this->view_chunk_x - this->draw_distance, (i64) 0
        );
        i64 viewed_end_x = std::min(
            this->view_chunk_x + this->draw_distance, (i64) this->width_chunks - 1
        );
        i64 viewed_start_z = std::max(
            this->view_chunk_z - this->draw_distance, (i64) 0
        );
        i64 viewed_end_z = std::min(
            this->view_chunk_z + this->draw_distance, (i64) this->height_chunks - 1
        );
        for(i64 chunk_x = viewed_start_x; chunk_x <= viewed_end_x; chunk_x += 1) {
            for(i64 chunk_z = viewed_start_z; chunk_z <= viewed_end_z; chunk_z += 1) {
                size_t chunk_i;
                if(!this->chunk_loaded(chunk_x, chunk_z, chunk_i)) {
                    this->loaded_chunks.push_back(this->load_chunk(chunk_x, chunk_z));
                    continue; 
                }
                LoadedChunk& chunk = this->loaded_chunks.at(chunk_i);
                if(chunk.modified) {
                    chunk = this->load_chunk(chunk_x, chunk_z);
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
                    const Building::TypeInfo& type = Building::types
                        .at((size_t) building.type);
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

    bool Terrain::valid_building_location(
        i64 tile_x, i64 tile_z, const Vec<3>& player_position, 
        const Building::TypeInfo& building_type
    ) const {
        i64 player_tile_x = (i64) player_position.x() / this->units_per_tile();
        i64 player_tile_z = (i64) player_position.z() / this->units_per_tile();
        i64 end_x = tile_x + (i64) building_type.width;
        i64 end_z = tile_z + (i64) building_type.height;
        if(tile_x < 0 || (u64) end_x > this->width_in_tiles()) { return false; }
        if(tile_z < 0 || (u64) end_z > this->height_in_tiles()) { return false; }
        i16 target = this->elevation_at((u64) tile_x, (u64) tile_z);
        for(i64 x = tile_x; x <= end_x; x += 1) {
            for(i64 z = tile_z; z <= end_z; z += 1) {
                i16 elevation = this->elevation_at((u64) x, (u64) z);
                if(elevation != target) { return false; }
            }
        }
        for(i64 x = tile_x; x < end_x; x += 1) {
            for(i64 z = tile_z; z < end_z; z += 1) {
                if(x == player_tile_x && z == player_tile_z) { return false; }
                if(this->building_at(x, z) != nullptr) { return false; }
                if(this->bridge_at(x, z) != nullptr) { return false; }
            }
        }
        if(target < 0) { return false; }
        return true;
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
        i64 start_x, i64 start_z, i64 end_x, i64 end_z
    ) {
        u64 min_x = (u64) std::max(start_x, (i64) 0);
        u64 min_z = (u64) std::max(start_z, (i64) 0);
        u64 max_x = std::min((u64) end_x, this->width_in_tiles() - 1);
        u64 max_z = std::min((u64) end_z, this->height_in_tiles() - 1);
        u64 min_ch_x = min_x / this->tiles_per_chunk();
        u64 min_ch_z = min_z / this->tiles_per_chunk();
        u64 max_ch_x = max_x / this->tiles_per_chunk();
        u64 max_ch_z = max_z / this->tiles_per_chunk();
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


    void Terrain::render_loaded_chunks(
        engine::Scene& scene, const Renderer& renderer,
        const engine::Window& window
    ) {
        const engine::Texture& ground_texture
            = scene.get<engine::Texture>(Terrain::ground_texture);
        std::unordered_map<Foliage::Type, std::vector<Mat<4>>> foliage_instances;
        std::unordered_map<Building::Type, std::vector<Mat<4>>> building_instances;
        for(LoadedChunk& chunk: this->loaded_chunks) {
            if(chunk.modified) {
                chunk = this->load_chunk(chunk.x, chunk.z);
            }
            Vec<3> chunk_offset = Vec<3>(chunk.x, 0, chunk.z)
                * this->chunk_tiles * this->tile_size;
            this->render_chunk_ground(
                chunk, ground_texture, chunk_offset, renderer
            );
            for(const auto& [foliage_type, instances]: chunk.foliage) {
                std::vector<Mat<4>>& f_inst = foliage_instances[foliage_type];
                f_inst.insert(f_inst.end(), instances.begin(), instances.end());
            }
            for(const auto& [building_type, instances]: chunk.buildings) {
                std::vector<Mat<4>>& b_inst = building_instances[building_type];
                b_inst.insert(b_inst.end(), instances.begin(), instances.end());
            }
        }
        for(const auto& [foliage_type, instances]: foliage_instances) {
            engine::Model& model = scene.get<engine::Model>(
                Foliage::types.at((size_t) foliage_type).model
            );
            renderer.render(model, instances);
        }
        for(const auto& [building_type, instances]: building_instances) {
            const Building::TypeInfo& type_info
                = Building::types[(size_t) building_type];
            type_info.render_buildings(window, scene, renderer, instances);
        }
        this->render_bridges(scene, renderer);
    }

    void Terrain::render_chunk_ground(
        LoadedChunk& loaded_chunk,
        const engine::Texture& ground_texture, 
        const Vec<3>& chunk_offset,
        const Renderer& renderer
    ) {
        renderer.render(
            loaded_chunk.terrain, ground_texture, 
            Mat<4>(),
            std::array { Mat<4>::translate(chunk_offset) },
            false
        );
    }

    void Terrain::render_bridges(
        engine::Scene& scene, const Renderer& renderer
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
        instances.resize(Bridge::types.size());
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
            const Bridge::TypeInfo& type = Bridge::types[type_id];
            engine::Model& model = scene.get<engine::Model>(type.model);
            renderer.render(model, instances[type_id]);
        }
    }

    static Vec<4> water_light_color = Vec<4>(245, 237, 186, 255) / 255;
    static Vec<4> water_base_color = Vec<4>(126, 196, 193, 255) / 255;
    static Vec<4> water_dark_color = Vec<4>(52, 133, 157, 255) / 255;

    void Terrain::render_water(
        engine::Scene& scene, const Renderer& renderer, 
        const engine::Window& window
    ) {
        const engine::Texture& normal_map = scene.get<engine::Texture>(Terrain::water_texture);
        engine::Shader& shader = scene.get<engine::Shader>(Terrain::water_shader);
        f64 scale_xz = 2 * (this->draw_distance + 1) * this->chunk_tiles * this->tile_size;
        Vec<3> offset = {
            (this->view_chunk_x + 0.5) * this->chunk_tiles * this->tile_size,
            0,
            (this->view_chunk_z + 0.5) * this->chunk_tiles * this->tile_size
        };
        shader.set_uniform("u_view_projection", renderer.compute_view_proj());
        shader.set_uniform("u_local_transf", Mat<4>::scale(Vec<3>(scale_xz, 1.0, scale_xz)));
        shader.set_uniform("u_model_transf", Mat<4>::translate(offset));
        shader.set_uniform("u_light_color", water_light_color);
        shader.set_uniform("u_base_color", water_base_color);
        shader.set_uniform("u_dark_color", water_dark_color);
        shader.set_uniform("u_nmap", normal_map);
        f64 units_to_nmap = 1.0 / normal_map.width() * 16; // 16 pixels per unit
        f64 nmap_scale = scale_xz * units_to_nmap;
        Vec<2> nmap_offset = offset.swizzle<2>("xz") * units_to_nmap;
        shader.set_uniform("u_nmap_scale", nmap_scale);
        shader.set_uniform("u_nmap_offset", nmap_offset);
        shader.set_uniform("u_time", window.time());
        renderer.set_fog_uniforms(shader);
        renderer.set_shadow_uniforms(shader);
        this->water_plane.render(shader, renderer.output().as_target());
    }


    Terrain::Terrain(
        const Serialized& serialized,
        i64 draw_distance, u64 tile_size, u64 chunk_tiles,
        const engine::Arena& buffer
    ) {
        this->width = serialized.width;
        this->height = serialized.height;
        this->tile_size = tile_size;
        this->chunk_tiles = chunk_tiles;
        this->draw_distance = draw_distance;
        this->view_chunk_x = 0;
        this->view_chunk_z = 0;
        buffer.copy_array_at_into(
            serialized.elevation_offset, serialized.elevation_count,
            this->elevation
        );
        this->width_chunks = (u64) ceil((f64) width / chunk_tiles);
        this->height_chunks = (u64) ceil((f64) height / chunk_tiles);
        std::span<const ChunkData::Serialized> chunks = buffer
            .array_at<ChunkData::Serialized>(
                serialized.chunk_offset, serialized.chunk_count
            );
        this->chunks.reserve(chunks.size());
        for(const ChunkData::Serialized& chunk: chunks) {
            this->chunks.push_back(ChunkData(this->chunk_tiles, chunk, buffer));
        }
        buffer.copy_array_at_into(
            serialized.bridge_offset, serialized.bridge_count,
            this->bridges
        );
        this->build_water_plane();
    }

    Terrain::ChunkData::ChunkData(
        u64 size_tiles,
        const ChunkData::Serialized& serialized, const engine::Arena& buffer
    ) {
        this->size_tiles = size_tiles;
        buffer.copy_array_at_into(
            serialized.foliage_offset, serialized.foliage_count,
            this->foliage
        );
        buffer.copy_array_at_into(
            serialized.building_offset, serialized.building_count,
            this->buildings
        );
        buffer.copy_array_at_into(
            serialized.paths_offset, serialized.paths_count,
            this->paths
        );
    }

    Terrain::ChunkData::Serialized Terrain::ChunkData::serialize(
        engine::Arena& buffer
    ) const {
        return {
            this->foliage.size(), buffer.alloc_array(this->foliage),
            this->buildings.size(), buffer.alloc_array(this->buildings),
            this->paths.size(), buffer.alloc_array(this->paths)
        };
    }

    Terrain::Serialized Terrain::serialize(engine::Arena& buffer) const {
        auto chunk_data = std::vector<ChunkData::Serialized>();
        chunk_data.reserve(this->chunks.size());
        for(const ChunkData& chunk: this->chunks) {
            chunk_data.push_back(chunk.serialize(buffer));
        }
        return {
            this->width, this->height,
            this->elevation.size(), buffer.alloc_array(this->elevation),
            this->chunks.size(), buffer.alloc_array(chunk_data),
            this->bridges.size(), buffer.alloc_array(this->bridges)
        };
    }

}