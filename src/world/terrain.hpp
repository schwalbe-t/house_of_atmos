
#pragma once

#include <engine/arena.hpp>
#include <engine/rng.hpp>
#include "../interactable.hpp"
#include "complex.hpp"
#include "buildings.hpp"
#include "foliage.hpp"
#include "bridge.hpp"

namespace houseofatmos::world {

    using namespace houseofatmos;
    using namespace houseofatmos::engine::math;


    struct World;


    struct Terrain {

        struct Serialized {
            u64 width, height;
            u64 elevation_count, elevation_offset;
            u64 chunk_count, chunk_offset;
            u64 bridge_count, bridge_offset;
        };


        static inline engine::Texture::LoadArgs ground_texture = {
            "res/terrain/ground.png"
        };

        static inline engine::Texture::LoadArgs water_texture = {
            "res/terrain/water.png"
        };

        static inline engine::Shader::LoadArgs water_shader = {
            "res/shaders/water_vert.glsl", "res/shaders/water_frag.glsl"
        };

        static const inline std::vector<engine::Mesh::Attrib> water_plane_attribs = {
            { engine::Mesh::F32, 3 }
        };


        struct LoadedChunk {
            i64 x, z; // in chunks relative to origin
            bool modified; // re-mesh in next render cycle
            engine::Mesh terrain; // terrain geometry
            engine::Mesh water; // water geometry
            std::unordered_map<Foliage::Type, std::vector<Mat<4>>> foliage;
            std::unordered_map<Building::Type, std::vector<Mat<4>>> buildings;
            std::vector<std::shared_ptr<Interactable>> interactables;
        };

        struct ChunkData {
            struct Serialized {
                u64 foliage_count, foliage_offset;
                u64 building_count, building_offset;
                u64 paths_count, paths_offset;
            };

            ChunkData(u64 size_tiles) {
                this->size_tiles = size_tiles;
                this->paths.resize(size_tiles * size_tiles);
            }
            ChunkData(
                u64 size_tiles, 
                const Serialized& serialized, const engine::Arena& buffer
            );

            std::vector<Foliage> foliage;
            std::vector<Building> buildings;
            std::vector<u8> paths;
            u64 size_tiles;

            void set_path_at(u64 rel_x, u64 rel_z, bool is_present) {
                this->paths.at(rel_x + this->size_tiles * rel_z) = is_present?
                    1 : 0;
            }
            bool path_at(u64 rel_x, u64 rel_z) const {
                return this->paths.at(rel_x + this->size_tiles * rel_z) != 0;
            }

            Serialized serialize(engine::Arena& buffer) const;
        };

        private:
        u64 width, height;
        u64 tile_size;
        u64 chunk_tiles;
        u64 width_chunks, height_chunks;
        i64 view_chunk_x, view_chunk_z;
        std::vector<LoadedChunk> loaded_chunks;
        // row-major 2D vector of each tile corner height
        // .size() = (width + 1) * (height + 1)
        std::vector<i16> elevation;
        // row-major 2D vector of chunks
        // .size() = width_chunks * height_chunks
        std::vector<ChunkData> chunks; 

        std::unordered_map<Foliage::Type, std::vector<Mat<4>>>
            collect_foliage_transforms(u64 chunk_x, u64 chunk_z) const;
        std::unordered_map<Building::Type, std::vector<Mat<4>>>
            collect_building_transforms(u64 chunk_x, u64 chunk_z) const;
        std::vector<std::shared_ptr<Interactable>> create_chunk_interactables(
            u64 chunk_x, u64 chunk_z, 
            Interactables* interactables, engine::Window& window, 
            const std::shared_ptr<World>& world
        ) const;
        Terrain::LoadedChunk load_chunk(
            i64 chunk_x, i64 chunk_z, 
            Interactables* interactables, engine::Window& window, 
            const std::shared_ptr<World>& world,
            bool in_bounds = true
        ) const;


        public:
        std::vector<Bridge> bridges;


        static void load_resources(engine::Scene& scene) {
            scene.load(engine::Texture::Loader(Terrain::ground_texture));
            scene.load(engine::Texture::Loader(Terrain::water_texture));
            scene.load(engine::Shader::Loader(Terrain::water_shader));
        }

        Terrain(u64 width, u64 height, u64 tile_size, u64 chunk_tiles) {
            if(tile_size * chunk_tiles > UINT8_MAX) {
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
            this->view_chunk_x = 0;
            this->view_chunk_z = 0;
            this->elevation.resize((width + 1) * (height + 1));
            this->width_chunks = (u64) ceil((f64) width / chunk_tiles);
            this->height_chunks = (u64) ceil((f64) height / chunk_tiles);
            this->chunks = std::vector<ChunkData>(
                width_chunks * height_chunks, ChunkData(chunk_tiles)
            );
        }
        
        Terrain(
            const Serialized& serialized, u64 tile_size, u64 chunk_tiles,
            const engine::Arena& buffer
        );

        u64 width_in_tiles() const { return this->width; }
        u64 height_in_tiles() const { return this->height; }
        u64 width_in_chunks() const { return this->width_chunks; }
        u64 height_in_chunks() const { return this->height_chunks; }
        u64 units_per_tile() const { return this->tile_size; }
        u64 tiles_per_chunk() const { return this->chunk_tiles; }
        i64 viewed_chunk_x() const { return this->view_chunk_x; }
        i64 viewed_chunk_z() const { return this->view_chunk_z; }

        i16& elevation_at(u64 x, u64 z) {
            return this->elevation.at(x + (this->width + 1) * z);
        }
        i16 elevation_at(u64 x, u64 z) const {
            return this->elevation.at(x + (this->width + 1) * z);
        }
        f64 elevation_at(const Vec<3>& pos) const;
        ChunkData& chunk_at(u64 chunk_x, u64 chunk_z) {
            return this->chunks.at(chunk_x + this->width_chunks * chunk_z);
        }
        const ChunkData& chunk_at(u64 chunk_x, u64 chunk_z) const {
            return this->chunks.at(chunk_x + this->width_chunks * chunk_z);
        }
        void reload_chunk_at(u64 chunk_x, u64 chunk_z) {
            auto chunk = this->loaded_chunk_at((i64) chunk_x, (i64) chunk_z);
            if(chunk != nullptr) { chunk->modified = true; }
        }
        LoadedChunk* loaded_chunk_at(i64 chunk_x, i64 chunk_z) {
            for(LoadedChunk& chunk: this->loaded_chunks) {
                if(chunk.x != chunk_x || chunk.z != chunk_z) { continue; }
                return &chunk;
            }
            return nullptr;
        }
        Building* building_at(
            i64 tile_x, i64 tile_z, 
            u64* chunk_x_out = nullptr, u64* chunk_z_out = nullptr
        );
        const Building* building_at(
            i64 tile_x, i64 tile_z, 
            u64* chunk_x_out = nullptr, u64* chunk_z_out = nullptr
        ) const;
        bool path_at(i64 tile_x, i64 tile_z) const {
            if(tile_x < 0 || tile_z < 0) { return false; }
            if((u64) tile_x >= this->width) { return false; }
            if((u64) tile_z >= this->height) { return false; }
            u64 chunk_x = (u64) tile_x / this->chunk_tiles;
            u64 chunk_z = (u64) tile_z / this->chunk_tiles;
            const Terrain::ChunkData& chunk = this->chunk_at(chunk_x, chunk_z);
            u64 rel_x = (u64) tile_x % this->chunk_tiles;
            u64 rel_z = (u64) tile_z % this->chunk_tiles;
            return chunk.path_at(rel_x, rel_z);
        }
        const Bridge* bridge_at(
            i64 tile_x, i64 tile_z, f64 closest_to_height = 0.0
        ) const;
        bool valid_building_location(
            i64 tile_x, i64 tile_z, const Vec<3>& player_position, 
            const Building::TypeInfo& building_type
        ) const;
        bool valid_player_position(
            const AbsCollider& player_collider, bool water_is_obstacle
        ) const;
        void remove_foliage_at(i64 tile_x, i64 tile_z);
        void adjust_area_foliage(i64 start_x, i64 start_z, i64 end_x, i64 end_z);

        std::pair<u64, u64> find_selected_terrain_tile(
            Vec<2> cursor_pos_ndc, const Renderer& renderer, Vec<3> tile_offset
        ) const;
        i64 compute_unemployment() const;

        void generate_elevation(
            u32 seed = (u32) random_init(), 
            f64 end_falloff_distance = 0.0,
            f64 falloff_target_height = 0.0
        );
        void generate_foliage(u32 seed = (u32) random_init());

        bool chunk_in_draw_distance(
            u64 chunk_x, u64 chunk_z, u64 draw_distance
        ) const;
        bool chunk_loaded(u64 chunk_x, u64 chunk_z, size_t& index) const;
        void load_chunks_around(
            const Vec<3>& position, u64 draw_distance,
            Interactables* interactables, engine::Window& window, 
            const std::shared_ptr<World>& world
        );

        engine::Mesh build_chunk_terrain_geometry(u64 chunk_x, u64 chunk_z) const;
        engine::Mesh build_chunk_water_geometry(i64 chunk_x, i64 chunk_z) const;
        Mat<4> building_transform(
            const Building& building, u64 chunk_x, u64 chunk_z
        ) const;    

        void render_loaded_chunks(
            engine::Scene& scene, const Renderer& renderer,
            const engine::Window& window
        );
        void render_water(
            engine::Scene& scene, const Renderer& renderer,
            const engine::Window& window
        );
        private:
        void render_chunk_ground(
            LoadedChunk& loaded_chunk,
            const engine::Texture& ground_texture, 
            const Vec<3>& chunk_offset,
            const Renderer& renderer
        );
        void render_bridges(
            engine::Scene& scene, const Renderer& renderer
        );

        public:
        Serialized serialize(engine::Arena& buffer) const;

    };

} 