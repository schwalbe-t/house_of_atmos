
#include "terrain.hpp"

namespace houseofatmos::outside {

    static f32 water_height = -0.5;
    static f64 sand_max_height = 0.0;
    static f64 stone_min_height_diff = 2.5;


    static Vec<3> compute_barycentric(
        const Vec<3>& a, const Vec<3>& b, const Vec<3>& c, const Vec<3>& p
    ) {
        Vec<3> v_0 = b - a;
        Vec<3> v_1 = c - a;
        Vec<3> v_2 = p - a;
        f64 d_00 = v_0.dot(v_0);
        f64 d_01 = v_0.dot(v_1);
        f64 d_11 = v_1.dot(v_1);
        f64 d_20 = v_2.dot(v_0);
        f64 d_21 = v_2.dot(v_1);
        f64 denom = (d_00 * d_11) - (d_01 * d_01);
        f64 r_y = ((d_11 * d_20) - (d_01 * d_21)) / denom;
        f64 r_z = ((d_00 * d_21) - (d_01 * d_20)) / denom;
        return Vec<3>(1.0 - r_y - r_z, r_y, r_z);
    }

    static bool compute_elevation(
        const Vec<3>& a, const Vec<3>& b, const Vec<3>& c, const Vec<3>& p,
        f64& elevation
    ) {
        Vec<3> bc = compute_barycentric(a, b, c, p);
        elevation = bc.x() * a.y() + bc.y() * b.y() + bc.z() * c.y();
        return bc.x() >= 0 && bc.y() >= 0 && bc.z() >= 0;
    }

    f64 Terrain::elevation_at(const Vec<3>& pos) {
        if(pos.x() < 0 || pos.z() < 0) { return 0.0; }
        u64 left = (u64) (pos.x() / this->tile_size);
        u64 top = (u64) (pos.z() / this->tile_size);
        u64 right = left + 1;
        u64 bottom = top + 1;
        if(left >= this->width || top >= this->height) { return 0.0; }
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
        f64 tl_br_height_diff = fabs(tl.y() - br.y());
        f64 tr_bl_height_diff = fabs(tr.y() - bl.y());
        f64 elev;
        if(tr_bl_height_diff > tl_br_height_diff) {
            // tl---tr
            //  | \ |
            // bl---br
            if(compute_elevation(tl, bl, br, pos, elev)) { return elev; }
            if(compute_elevation(tl, br, tr, pos, elev)) { return elev; }
            return elev; // on boundary between the two triangles, return either
        } else {
            // tl---tr
            //  | / |
            // bl---br
            if(compute_elevation(tl, bl, tr, pos, elev)) { return elev; }
            if(compute_elevation(tr, bl, br, pos, elev)) { return elev; }
            return elev; // on boundary between the two triangles, return either
        }
    }


    void Terrain::generate_elevation(u32 seed) {
        for(u64 x = 0; x <= this->width; x += 1) {
            for(u64 z = 0; z <= this->height; z += 1) {
                f64 height = 0.0;
                height += perlin_noise(seed, Vec<2>(x / 1.0, z / 1.0));
                height += perlin_noise(seed, Vec<2>((f64) x / 40.0, (f64) z / 40.0)) * 10
                    * (perlin_noise(seed, Vec<2>(x / 128.0, z / 128.0)) * 5);
                this->elevation_at(x, z) = floor(height);
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
                for(size_t type_i = 0; type_i < Foliage::types.size(); type_i += 1) {
                    const Foliage::TypeInfo& type = Foliage::types.at(type_i);
                    f64 chance = type.spawn_chance * slope_factor;
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
        p.clear();
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
        engine::Mesh& dest
    ) {
        f64 min_height = std::min(pos_a.y(), std::min(pos_b.y(), pos_c.y()));
        f64 max_height = std::max(pos_a.y(), std::max(pos_b.y(), pos_c.y()));
        f64 height_diff = max_height - min_height;
        Vec<2> uv_offset = (min_height < sand_max_height
            ? Vec<2>(0, 0.5)
            : (height_diff > stone_min_height_diff
                ? Vec<2>(0.5, 0)
                : Vec<2>(0, 0)
            )
        );
        Vec<3> normal = compute_normal_ccw(pos_a, pos_b, pos_c);
        dest.add_element(
            put_terrain_vertex(pos_a, uv_a + uv_offset, normal, dest),
            put_terrain_vertex(pos_b, uv_b + uv_offset, normal, dest),
            put_terrain_vertex(pos_c, uv_c + uv_offset, normal, dest)
        );
    }

    engine::Mesh Terrain::build_chunk_geometry(u64 chunk_x, u64 chunk_z) {
        auto geometry = engine::Mesh(Renderer::mesh_attribs);
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
                Vec<2> uv_bl = { 0.0, 0.0 };
                Vec<2> uv_br = { 0.5, 0.0 };
                Vec<2> uv_tl = { 0.0, 0.5 };
                Vec<2> uv_tr = { 0.5, 0.5 };
                if(tr_bl_height_diff > tl_br_height_diff) {
                    // tl---tr
                    //  | \ |
                    // bl---br
                    put_terrain_element_ccw(
                        pos_tl, pos_bl, pos_br, uv_tl, uv_bl, uv_br, geometry
                    );
                    put_terrain_element_ccw(
                        pos_tl, pos_br, pos_tr, uv_tl, uv_br, uv_tr, geometry
                    );
                } else {
                    // tl---tr
                    //  | / |
                    // bl---br
                    put_terrain_element_ccw(
                        pos_tl, pos_bl, pos_tr, uv_tl, uv_bl, uv_tr, geometry
                    );
                    put_terrain_element_ccw(
                        pos_tr, pos_bl, pos_br, uv_tr, uv_bl, uv_br, geometry
                    );
                }
            }
        }
        geometry.submit();
        return geometry;
    }

    std::unordered_map<Foliage::Type, std::vector<Mat<4>>>
        Terrain::collect_foliage_transforms(u64 chunk_x, u64 chunk_z) {
        std::unordered_map<Foliage::Type, std::vector<Mat<4>>> instances;
        ChunkData& chunk_data = this->chunk_at(chunk_x, chunk_z);
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

    Terrain::LoadedChunk Terrain::load_chunk(u64 chunk_x, u64 chunk_z) {
        return {
            chunk_x, chunk_z, false, 
            build_chunk_geometry(chunk_x, chunk_z),
            collect_foliage_transforms(chunk_x, chunk_z)
        };
    }

    bool Terrain::chunk_in_draw_distance(u64 chunk_x, u64 chunk_z) const {
        i64 diff_x = (i64) chunk_x - this->view_chunk_x;
        i64 diff_z = (i64) chunk_z - this->view_chunk_z;
        u64 mh_dist = llabs(diff_x) + llabs(diff_z);
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


    void Terrain::render_loaded_chunks(
        engine::Scene& scene, const Renderer& renderer
    ) {
        const engine::Texture& ground_texture
            = scene.get<engine::Texture>(Terrain::ground_texture);
        for(LoadedChunk& chunk: this->loaded_chunks) {
            if(chunk.modified) {
                chunk = this->load_chunk(chunk.x, chunk.z);
            }
            Vec<3> chunk_offset = Vec<3>(chunk.x, 0, chunk.z)
                * this->chunk_tiles * this->tile_size;
            chunk.render_ground(scene, renderer, ground_texture, chunk_offset);
            const ChunkData& data = this->chunk_at(chunk.x, chunk.z);
            this->render_chunk_features(chunk, chunk_offset, scene, renderer);
        }
        this->render_water(scene, renderer);
    }

    void Terrain::LoadedChunk::render_ground(
        engine::Scene& scene, const Renderer& renderer,
        const engine::Texture& ground_texture, const Vec<3>& chunk_offset
    ) {
        renderer.render(
            this->terrain, ground_texture, 
            Mat<4>(),
            std::array { Mat<4>::translate(chunk_offset) }
        );
    }

    void Terrain::render_chunk_features(
        LoadedChunk& loaded_chunk, const Vec<3>& chunk_offset,
        engine::Scene& scene, const Renderer& renderer
    ) {
        const ChunkData& data = this->chunk_at(loaded_chunk.x, loaded_chunk.z);
        for(const auto& [foliage_type, instances]: loaded_chunk.foliage) {
            engine::Model& model = scene.get<engine::Model>(
                Foliage::types.at((size_t) foliage_type).model
            );
            renderer.render(model, instances);
        }
        for(const Building& building: data.buildings) {
            const Building::TypeInfo& type = building.get_type_info();
            Vec<3> offset = chunk_offset 
                + Vec<3>(building.x, 0, building.z) * this->tile_size
                + Vec<3>(
                    type.width * this->tile_size / 2, 0, 
                    type.height * this->tile_size / 2
                );
            offset.y() = this->elevation_at(
                loaded_chunk.x * this->chunk_tiles + building.x,
                loaded_chunk.z * this->chunk_tiles + building.z
            );
            engine::Model& model = scene.get<engine::Model>(
                building.get_type_info().model
            );
            renderer.render(model, std::array { Mat<4>::translate(offset) });
        }
    }

    static Vec<4> water_base_color = Vec<4>(126, 196, 193, 230) / 255;

    void Terrain::render_water(engine::Scene& scene, const Renderer& renderer) {
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
        shader.set_uniform("u_base_color", water_base_color);
        this->water_plane.render(shader, renderer.output(), 1, true);
    }

}