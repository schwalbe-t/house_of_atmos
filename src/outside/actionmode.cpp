
#include "actionmode.hpp"

namespace houseofatmos::outside {

    void ActionMode::choose_current(
        const engine::Window& window, Terrain& terrain,
        std::unique_ptr<ActionMode>& current
    ) {
        for(size_t type_i = 0; type_i < ActionMode::keys.size(); type_i += 1) {
            engine::Key key = ActionMode::keys.at(type_i);
            ActionMode::Type type = (ActionMode::Type) type_i;
            if(!window.was_pressed(key)) { continue; }
            if(current->get_type() == type) {
                current = std::make_unique<DefaultMode>();
                break;
            }
            switch(type) {
                case Default: current = std::make_unique<DefaultMode>(); break;
                case Terraform: current = std::make_unique<TerraformMode>(terrain); break;
                default: engine::warning(
                    "Unhandled 'ActionMode::Type' in 'ActionMode::choose_current'"
                );
            }
            break;
        }
    }



    static const i64 chunk_sel_range = 1;

    static std::pair<u64, u64> find_selected_terrain_vertex(
        Vec<2> cursor_pos_ndc, const Mat<4>& view_proj, const Terrain& terrain
    ) {
        i64 start_x = (terrain.viewed_chunk_x() - chunk_sel_range) 
            * (i64) terrain.tiles_per_chunk();
        i64 end_x = (terrain.viewed_chunk_x() + chunk_sel_range) 
            * (i64) terrain.tiles_per_chunk();
        i64 start_z = (terrain.viewed_chunk_z() - chunk_sel_range) 
            * (i64) terrain.tiles_per_chunk();
        i64 end_z = (terrain.viewed_chunk_z() + chunk_sel_range) 
            * (i64) terrain.tiles_per_chunk();
        std::pair<u64, u64> current;
        f64 current_dist = INFINITY;
        for(
            i64 tile_x = std::max(start_x, (i64) 0); 
            tile_x < std::min(end_x, (i64) terrain.width_in_tiles());
            tile_x += 1
        ) {
            for(
                i64 tile_z = std::max(start_z, (i64) 0); 
                tile_z < std::min(end_z, (i64) terrain.width_in_tiles());
                tile_z += 1
            ) {
                Vec<3> pos = Vec<3>(tile_x, 0, tile_z) * terrain.units_per_tile();
                pos.y() = terrain.elevation_at((u64) tile_x, (u64) tile_z);
                Vec<4> pos_ndc = view_proj * pos.with(1.0);
                pos_ndc = pos_ndc / pos_ndc.w(); // perspective divide
                f64 dist = (pos_ndc.swizzle<2>("xy") - cursor_pos_ndc).len();
                if(dist > current_dist) { continue; }
                current = { (u64) tile_x, (u64) tile_z };
                current_dist = dist;
            }
        }
        return current;
    }

    static void remove_chunk_foliage_on_tile(
        Terrain::ChunkData& chunk, u64 chunk_x, u64 chunk_z, 
        u64 tile_x, u64 tile_z,
        u64 tiles_per_chunk, u64 units_per_tile
    ) {
        for(size_t foliage_i = 0; foliage_i < chunk.foliage.size();) {
            const Foliage& foliage = chunk.foliage.at(foliage_i);
            u64 foliage_tile_x = chunk_x * tiles_per_chunk
                + (u64) foliage.x / units_per_tile;
            u64 foliage_tile_z = chunk_z * tiles_per_chunk
                + (u64) foliage.z / units_per_tile;
            if(foliage_tile_x != tile_x || foliage_tile_z != tile_z) {
                foliage_i += 1;
                continue;
            }
            chunk.foliage.erase(chunk.foliage.begin() + foliage_i);
        }
    }

    void TerraformMode::update(const engine::Window& window, const Renderer& renderer) {
        bool modified_terrain = window.was_pressed(engine::Button::Left)
            || window.was_pressed(engine::Button::Right);
        if(modified_terrain) {
            auto [tile_x, tile_z] = find_selected_terrain_vertex(
                window.cursor_pos_ndc(), renderer.compute_view_proj(), this->terrain
            );
            i16& elevation = this->terrain.elevation_at(tile_x, tile_z);
            if(window.was_pressed(engine::Button::Left)) {
                elevation -= 1;
            }
            if(window.was_pressed(engine::Button::Right)) {
                elevation += 1;
            }
            for(i64 offset_x = -1; offset_x <= 0; offset_x += 1) {
                for(i64 offset_z = -1; offset_z <= 0; offset_z += 1) {
                    u64 u_tile_x = std::min(
                        (u64) std::max((i64) tile_x + offset_x, (i64) 0),
                        this->terrain.width_in_tiles()
                    );
                    u64 u_tile_z = std::min(
                        (u64) std::max((i64) tile_z + offset_z, (i64) 0),
                        this->terrain.height_in_tiles()
                    );
                    u64 u_chunk_x = u_tile_x / this->terrain.tiles_per_chunk();
                    u64 u_chunk_z = u_tile_z / this->terrain.tiles_per_chunk();
                    Terrain::ChunkData& u_chunk
                        = this->terrain.chunk_at(u_chunk_x, u_chunk_z);
                    remove_chunk_foliage_on_tile(
                        u_chunk, u_chunk_x, u_chunk_z, u_tile_x, u_tile_z,
                        this->terrain.tiles_per_chunk(),
                        this->terrain.units_per_tile()
                    );
                    this->terrain.reload_chunk_at(u_chunk_x, u_chunk_z);
                }
            }
        }
    }

    void TerraformMode::render(engine::Scene& scene, const Renderer& renderer) {
        (void) scene;
        (void) renderer;
    }

}