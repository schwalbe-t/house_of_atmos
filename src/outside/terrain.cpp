
#include "terrain.hpp"

namespace houseofatmos::outside {

    f64 Terrain::elevation_at(const Vec<3>& pos) {
        engine::error("'Terrain::elevation_at': not yet implemented");
    }


    void Terrain::generate_elevation(u32 seed) {
        engine::warning("'Terrain::generate_elevation': not yet implemented");
    }

    void Terrain::generate_foliage(u32 seed) {
        engine::warning("'Terrain::generate_foliage': not yet implemented");
    }


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
                chunk.terrain.start_vertex();
                chunk.terrain.put_f32({ // position
                    (f32) left * this->tile_size, 
                    (f32) this->elevation_at(left, top), 
                    (f32) top * this->tile_size 
                });
                chunk.terrain.put_f32({ 0, 0 }); // uv
                chunk.terrain.put_f32({ 0, 1, 0 }); // normal
                chunk.terrain.put_u8({ 0, 0, 0, 0 }); // joint indices
                chunk.terrain.put_f32({ 1, 0, 0, 0 }); // weights
                u16 tl = chunk.terrain.complete_vertex();
                chunk.terrain.start_vertex();
                chunk.terrain.put_f32({ // position
                    (f32) right * this->tile_size, 
                    (f32) this->elevation_at(right, top), 
                    (f32) top * this->tile_size
                });
                chunk.terrain.put_f32({ 0, 0 }); // uv
                chunk.terrain.put_f32({ 0, 1, 0 }); // normal
                chunk.terrain.put_u8({ 0, 0, 0, 0 }); // joint indices
                chunk.terrain.put_f32({ 1, 0, 0, 0 }); // weights
                u16 tr = chunk.terrain.complete_vertex();
                chunk.terrain.start_vertex();
                chunk.terrain.put_f32({ // position
                    (f32) left * this->tile_size, 
                    (f32) this->elevation_at(left, bottom), 
                    (f32) bottom * this->tile_size
                });
                chunk.terrain.put_f32({ 0, 0 }); // uv
                chunk.terrain.put_f32({ 0, 1, 0 }); // normal
                chunk.terrain.put_u8({ 0, 0, 0, 0 }); // joint indices
                chunk.terrain.put_f32({ 1, 0, 0, 0 }); // weights
                u16 bl = chunk.terrain.complete_vertex();
                chunk.terrain.start_vertex();
                chunk.terrain.put_f32({ // position
                    (f32) right * this->tile_size,
                    (f32) this->elevation_at(right, bottom),
                    (f32) bottom * this->tile_size
                });
                chunk.terrain.put_f32({ 0, 0 }); // uv
                chunk.terrain.put_f32({ 0, 1, 0 }); // normal
                chunk.terrain.put_u8({ 0, 0, 0, 0 }); // joint indices
                chunk.terrain.put_f32({ 1, 0, 0, 0 }); // weights
                u16 br = chunk.terrain.complete_vertex();
                // tl --- tr
                //  |  \  |
                // bl --- br
                chunk.terrain.add_element(tl, bl, br);
                chunk.terrain.add_element(tl, br, tr);
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
        engine::Scene& scene, const Renderer& renderer,
        const engine::Texture& ground_texture
    ) {
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
            Vec<3> offset = chunk_offset 
                + Vec<3>(building.x, 0, building.z) * this->tile_size;
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