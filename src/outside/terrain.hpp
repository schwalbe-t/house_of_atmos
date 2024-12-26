
#pragma once

#include "buildings.hpp"
#include "foliage.hpp"

namespace houseofatmos::outside {

    using namespace houseofatmos;
    using namespace houseofatmos::engine::math;


    struct Terrain {

        struct LoadedChunk {
            u64 x, z; // in chunks relative to origin
            bool modified; // re-mesh in next render cycle
            engine::Mesh terrain;

            void render_ground(
                engine::Scene& scene, const Renderer& renderer,
                const engine::Texture& ground_texture, const Vec<3>& chunk_offset
            );
        };

        struct ChunkData {
            std::vector<Foliage> foliage;
            std::vector<Building> buildings;
        };

        private:
        u64 width, height;
        u64 tile_size;
        u64 chunk_tiles;
        i64 draw_distance;
        u64 width_chunks, height_chunks;
        i64 view_chunk_x, view_chunk_z;
        std::vector<LoadedChunk> loaded_chunks;

        Terrain::LoadedChunk load_chunk(u64 chunk_x, u64 chunk_y);


        public:
        // row-major 2D vector of each tile corner height
        // .size() = (width + 1) * (height + 1)
        std::vector<f64> elevation;
        // row-major 2D vector of chunks
        // .size() = width_chunks * height_chunks
        std::vector<ChunkData> chunks; 


        Terrain(
            u64 width, u64 height, u64 tile_size = 10, u64 chunk_tiles = 8,
            i64 draw_distance = 2
        ) {
            if(tile_size * chunk_tiles > UINT8_MAX + 1) {
                engine::warning("The product of tile_size and chunk_tiles must "
                    "not exceed 256, as this may result in incosistent foliage "
                    "placement. Given values are tile_size=" + std::to_string(tile_size)
                    + ", chunk_tiles=" + std::to_string(chunk_tiles)
                );
            }
            this->width = width;
            this->height = height;
            this->tile_size = tile_size;
            this->chunk_tiles = chunk_tiles;
            this->draw_distance = draw_distance;
            this->view_chunk_x = 0;
            this->view_chunk_z = 0;
            this->elevation.resize((width + 1) * (height + 1));
            this->width_chunks = (u64) ceil((f64) width / chunk_tiles);
            this->height_chunks = (u64) ceil((f64) height / chunk_tiles);
            this->chunks.resize(width_chunks * height_chunks);
        }

        f64& elevation_at(u64 x, u64 z) {
            return this->elevation.at(x + (this->width + 1) * z);
        }
        f64 elevation_at(const Vec<3>& pos);
        ChunkData& chunk_at(u64 chunk_x, u64 chunk_z) {
            return this->chunks.at(chunk_x + this->width_chunks * chunk_z);
        }

        void generate_elevation(u32 seed);
        void generate_foliage(u32 seed);

        bool chunk_in_draw_distance(u64 chunk_x, u64 chunk_z) const;
        bool chunk_loaded(u64 chunk_x, u64 chunk_z, size_t& index) const;
        void load_chunks_around(const Vec<3>& position);

        void render_loaded_chunks(
            engine::Scene& scene, const Renderer& renderer,
            const engine::Texture& texture
        );
        void render_chunk_features(
            LoadedChunk& loaded_chunk, const Vec<3>& chunk_offset,
            engine::Scene& scene, const Renderer& renderer
        );

    };

}