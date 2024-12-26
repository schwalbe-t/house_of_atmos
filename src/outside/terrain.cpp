
#include "terrain.hpp"

namespace houseofatmos::outside {

    f64 Terrain::elevation_at(const Vec<3>& pos) {
        engine::error("'Terrain::elevation_at': not yet implemented");
    }


    void Terrain::generate_elevation(u32 seed) {
        for(u64 x = 0; x <= this->width; x += 1) {
            for(u64 z = 0; z <= this->height; z += 1) {
                f64 height = 0.0;
                height += perlin_noise(seed, Vec<2>(x * 2.0, z * 2.0));
                height += perlin_noise(seed, Vec<2>((f64) x / 20.0, (f64) z / 20.0)) * 10
                    * (perlin_noise(seed, Vec<2>(x / 64.0, z / 64.0)) * 5);
                this->elevation_at(x, z) = floor(height * this->tile_size)
                    / (f64) this->tile_size;
            }
        }
    }

    void Terrain::generate_foliage(u32 seed) {
        engine::warning("'Terrain::generate_foliage': not yet implemented");
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
        Vec<3> normal = compute_normal_ccw(pos_a, pos_b, pos_c);
        dest.add_element(
            put_terrain_vertex(pos_a, uv_a, normal, dest),
            put_terrain_vertex(pos_b, uv_b, normal, dest),
            put_terrain_vertex(pos_c, uv_c, normal, dest)
        );
    }

    static f64 sand_max_height = 0.0;
    static f64 stone_min_height_diff = 4;

    Terrain::LoadedChunk Terrain::load_chunk(u64 chunk_x, u64 chunk_z) {
        LoadedChunk chunk = {
            chunk_x, chunk_z, false, engine::Mesh(Renderer::mesh_attribs) 
        };
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
                f64 height_diff = std::max(tl_br_height_diff, tr_bl_height_diff);
                f64 min_height = Vec<4>(
                    pos_tl.y(), pos_tr.y(), pos_bl.y(), pos_br.y()
                ).min();
                Vec<2> uv_bl = (min_height < sand_max_height
                    ? Vec<2>(0, 0.5)
                    : (height_diff > stone_min_height_diff
                        ? Vec<2>(0.5, 0)
                        : Vec<2>(0, 0)
                    )
                );
                Vec<2> uv_br = uv_bl + Vec<2>(0.5, 0);
                Vec<2> uv_tl = uv_bl + Vec<2>(0, 0.5);
                Vec<2> uv_tr = uv_bl + Vec<2>(0.5, 0.5);
                if(tr_bl_height_diff > tl_br_height_diff) {
                    // tl --- tr
                    //  |  \  |
                    // bl --- br
                    put_terrain_element_ccw(
                        pos_tl, pos_bl, pos_br, uv_tl, uv_bl, uv_br, chunk.terrain
                    );
                    put_terrain_element_ccw(
                        pos_tl, pos_br, pos_tr, uv_tl, uv_br, uv_tr, chunk.terrain
                    );
                } else {
                    // tl --- tr
                    //  |  /  |
                    // bl --- br
                    put_terrain_element_ccw(
                        pos_tl, pos_bl, pos_tr, uv_tl, uv_bl, uv_tr, chunk.terrain
                    );
                    put_terrain_element_ccw(
                        pos_tr, pos_bl, pos_br, uv_tr, uv_bl, uv_br, chunk.terrain
                    );
                }
            }
        }
        chunk.terrain.submit();
        return chunk;
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
            const ChunkData& data = this->chunk_at(chunk.x, chunk.z);
            Vec<3> chunk_offset = Vec<3>(chunk.x, 0, chunk.z)
                * this->chunk_tiles * this->tile_size;
            chunk.render_ground(scene, renderer, ground_texture, chunk_offset);
            this->render_chunk_features(chunk, chunk_offset, scene, renderer);
        }
    }

    void Terrain::LoadedChunk::render_ground(
        engine::Scene& scene, const Renderer& renderer,
        const engine::Texture& ground_texture, const Vec<3>& chunk_offset
    ) {
        renderer.render(
            this->terrain, ground_texture, Mat<4>::translate(chunk_offset)
        );
    }

    void Terrain::render_chunk_features(
        LoadedChunk& loaded_chunk, const Vec<3>& chunk_offset,
        engine::Scene& scene, const Renderer& renderer
    ) {
        const ChunkData& data = this->chunk_at(loaded_chunk.x, loaded_chunk.z);
        for(const Foliage& foliage: data.foliage) {
            Vec<3> offset = chunk_offset
                + Vec<3>(foliage.x, 0, foliage.z);
            offset.y() = this->elevation_at(offset);
            Mat<4> model_transform = Mat<4>::translate(offset)
                * Mat<4>::rotate_y(foliage.rotation);
            engine::Model& model = scene.get<engine::Model>(
                foliage.get_type_model()
            );
            renderer.render(model, model_transform);
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
            renderer.render(model, Mat<4>::translate(offset));
        }
    }

}