
#include "common.hpp"

namespace houseofatmos::world {

    namespace ui = houseofatmos::engine::ui;


    void PathingMode::update_overlay() {
        auto overlay = engine::Mesh { { engine::Mesh::F32, 3 } };
        u64 left = this->selected_tile_x;
        u64 right = left + 1;
        u64 top = this->selected_tile_z;
        u64 bottom = top + 1;
        overlay.start_vertex();
        overlay.put_f32({
            (f32) (left * this->world->terrain.units_per_tile()),
            (f32) this->world->terrain.elevation_at(left, top),
            (f32) (top * this->world->terrain.units_per_tile())
        });
        u16 tl = overlay.complete_vertex();
        overlay.start_vertex();
        overlay.put_f32({
            (f32) (right * this->world->terrain.units_per_tile()),
            (f32) this->world->terrain.elevation_at(right, top),
            (f32) (top * this->world->terrain.units_per_tile())
        });
        u16 tr = overlay.complete_vertex();
        overlay.start_vertex();
        overlay.put_f32({
            (f32) (left * this->world->terrain.units_per_tile()),
            (f32) this->world->terrain.elevation_at(left, bottom),
            (f32) (bottom * this->world->terrain.units_per_tile())
        });
        u16 bl = overlay.complete_vertex();
        overlay.start_vertex();
        overlay.put_f32({
            (f32) (right * this->world->terrain.units_per_tile()),
            (f32) this->world->terrain.elevation_at(right, bottom),
            (f32) (bottom * this->world->terrain.units_per_tile())
        });
        u16 br = overlay.complete_vertex();
        // tl---tr
        //  | \ |
        // bl---br
        overlay.add_element(tl, bl, br);
        overlay.add_element(tl, br, tr);
        this->overlay = std::move(overlay);
    }

    static bool valid_path_location(
        u64 tile_x, u64 tile_z, Terrain& terrain
    ) {
        return terrain.elevation_at(tile_x, tile_z) >= 0
            && terrain.elevation_at(tile_x + 1, tile_z) >= 0
            && terrain.elevation_at(tile_x, tile_z + 1) >= 0
            && terrain.elevation_at(tile_x + 1, tile_z + 1) >= 0;
    }

    static const u64 path_placement_cost = 10;
    static const u64 path_removal_refund = 5;

    void PathingMode::update(
        const engine::Window& window, engine::Scene& scene, 
        const Renderer& renderer
    ) {
        (void) scene;
        this->speaker.update();
        if(!this->permitted) { return; }
        auto [tile_x, tile_z] = this->world->terrain.find_selected_terrain_tile(
            window.cursor_pos_ndc(), renderer, Vec<3>(0.5, 0, 0.5)
        );
        bool selected_changed = this->selected_tile_x != tile_x
            || this->selected_tile_z != tile_z;
        this->selected_tile_x = tile_x;
        this->selected_tile_z = tile_z;
        if(selected_changed) { this->update_overlay(); }
        u64 chunk_x = tile_x / this->world->terrain.tiles_per_chunk();
        u64 chunk_z = tile_z / this->world->terrain.tiles_per_chunk();
        u64 rel_x = tile_x % this->world->terrain.tiles_per_chunk();
        u64 rel_z = tile_z % this->world->terrain.tiles_per_chunk();
        Terrain::ChunkData& chunk = this->world->terrain.chunk_at(chunk_x, chunk_z);
        bool has_path = chunk.path_at(rel_x, rel_z);
        bool place_path = !has_path 
            && window.is_down(engine::Button::Left)
            && !this->ui.is_hovered_over()
            && valid_path_location(tile_x, tile_z, this->world->terrain)
            && this->world->balance.pay_coins(path_placement_cost, this->toasts);
        if(place_path) {
            chunk.set_path_at(rel_x, rel_z, true);
            this->world->terrain.remove_foliage_at((i64) tile_x, (i64) tile_z);
            this->world->terrain.reload_chunk_at(chunk_x, chunk_z);
            this->world->carriages.reset(&this->toasts);
            this->speaker.position = Vec<3>(tile_x, 0, tile_z)
                * this->world->terrain.units_per_tile()
                + Vec<3>(0, this->world->terrain.elevation_at(tile_x, tile_z), 0);
            this->speaker.play(scene.get(sound::terrain_mod));
        } else if(has_path && window.is_down(engine::Button::Right)) {
            chunk.set_path_at(rel_x, rel_z, false);
            this->world->terrain.reload_chunk_at(chunk_x, chunk_z);
            this->world->carriages.reset(&this->toasts);
            this->world->balance.add_coins(path_removal_refund, this->toasts);
            this->speaker.position = Vec<3>(tile_x, 0, tile_z)
                * this->world->terrain.units_per_tile()
                + Vec<3>(0, this->world->terrain.elevation_at(tile_x, tile_z), 0);
            this->speaker.play(scene.get(sound::terrain_mod));
        }
    }

    void PathingMode::render(
        const engine::Window& window, engine::Scene& scene, 
        Renderer& renderer
    ) {
        (void) window;
        if(!this->permitted) { return; }
        if(!this->overlay.has_value()) { return; }
        engine::Shader& path_overlay_shader = scene
            .get(ActionMode::path_overlay_shader);
        path_overlay_shader.set_uniform(
            "u_view_proj", renderer.compute_view_proj()
        );
        path_overlay_shader.set_uniform(
            "u_tile_size", (f32) this->world->terrain.units_per_tile()
        );
        this->overlay->render(
            path_overlay_shader,
            renderer.output().as_target(),
            1,
            engine::FaceCulling::Disabled,
            engine::DepthTesting::Disabled
        );
    }
    
}