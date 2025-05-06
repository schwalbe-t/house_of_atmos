
#include "common.hpp"
#include <algorithm>

namespace houseofatmos::world {

    namespace ui = houseofatmos::engine::ui;


    template<typename M>
    static void remove_agent_stops(M& manager, ComplexId c) {
        for(auto& agent: manager.agents) {
            auto new_end = std::remove_if(
                agent.schedule.begin(), agent.schedule.end(),
                [c](const auto& stop) { return stop.target.index == c.index; }
            );
            agent.schedule.erase(new_end, agent.schedule.end());
        }
    }

    static const f64 demolition_refund_factor = 0.25;
    static const u64 track_removal_refund = 50;

    void DemolitionMode::attempt_demolition(engine::Scene& scene) {
        switch(this->selection.type) {
            case Selection::None: return;
            case Selection::Building: {
                const Selection::BuildingSelection& building 
                    = this->selection.value.building;
                const Building::TypeInfo& b_type 
                    = building.selected->get_type_info();
                if(!b_type.destructible) {
                    this->toasts.add_error("toast_indestructible", {});
                    return;
                }
                this->speaker.position = tile_bounded_position(
                    building.tile_x, building.tile_z, 
                    building.tile_x + b_type.width, 
                    building.tile_z + b_type.height,
                    this->world->player.character.position,
                    this->world->terrain
                );
                Terrain::ChunkData& chunk = this->world->terrain
                    .chunk_at(building.chunk_x, building.chunk_z);
                if(building.selected->complex.has_value()) {
                    u64 actual_x = building.selected->x
                        + building.chunk_x * this->world->terrain.tiles_per_chunk();
                    u64 actual_z = building.selected->z
                        + building.chunk_z * this->world->terrain.tiles_per_chunk();
                    ComplexId complex_id = *building.selected->complex;
                    Complex& complex = this->world->complexes.get(complex_id);
                    complex.remove_member(actual_x, actual_z);
                    if(complex.member_count() == 0) {
                        remove_agent_stops(this->world->carriages, complex_id);
                        remove_agent_stops(this->world->trains, complex_id);
                        remove_agent_stops(this->world->boats, complex_id);
                        this->world->complexes.delete_complex(complex_id);
                    }
                }
                size_t building_idx = building.selected - chunk.buildings.data();
                chunk.buildings.erase(chunk.buildings.begin() + building_idx);
                u64 refunded = (u64) ((f64) b_type.cost * demolition_refund_factor);
                this->world->balance.add_coins(refunded, this->toasts);
                for(i64 ch_o_x = -1; ch_o_x <= 1; ch_o_x += 1) {
                    for(i64 ch_o_z = -1; ch_o_z <= 1; ch_o_z += 1) {
                        i64 ch_x = (i64) building.chunk_x + ch_o_x;
                        i64 ch_z = (i64) building.chunk_z + ch_o_z;
                        bool in_bounds = ch_x >= 0 && ch_z >= 0
                            && (u64) ch_x < this->world->terrain.width_in_chunks()
                            && (u64) ch_z < this->world->terrain.height_in_chunks();
                        if(!in_bounds) { continue; }
                        this->world->terrain
                            .reload_chunk_at((u64) ch_x, (u64) ch_z);
                    }
                }
                this->world->carriages.reset(&this->toasts);
                this->world->populations
                    .reset(this->world->terrain, &this->toasts);
                this->selection.type = Selection::None;
                this->speaker.play(scene.get(sound::demolish));
                return;
            }
            case Selection::Bridge: {
                const Bridge* bridge = this->selection.value.bridge;
                const Bridge::TypeInfo& b_type = bridge->get_type_info();
                this->speaker.position = tile_bounded_position(
                    bridge->start_x, bridge->start_z, 
                    bridge->end_x, bridge->end_z,
                    this->world->player.character.position,
                    this->world->terrain
                );
                u64 tiles_per_chunk = this->world->terrain.tiles_per_chunk();
                u64 min_ch_x = bridge->start_x / tiles_per_chunk;
                u64 min_ch_z = bridge->start_z / tiles_per_chunk;
                u64 max_ch_x = bridge->end_x / tiles_per_chunk;
                u64 max_ch_z = bridge->end_z / tiles_per_chunk;
                for(u64 ch_x = min_ch_x; ch_x <= max_ch_x; ch_x += 1) {
                    for(u64 ch_z = min_ch_z; ch_z <= max_ch_z; ch_z += 1) {
                        Terrain::ChunkData& chunk = this->world->terrain
                            .chunk_at(ch_x, ch_z);
                        for(size_t tp_i = 0; tp_i < chunk.track_pieces.size();) {
                            const TrackPiece& tp = chunk.track_pieces[tp_i];
                            u64 tp_x = ch_x * tiles_per_chunk + tp.x;
                            u64 tp_z = ch_z * tiles_per_chunk + tp.z;
                            bool is_affected = tp_x >= bridge->start_x 
                                && tp_x <= bridge->end_x
                                && tp_z >= bridge->start_z 
                                && tp_z <= bridge->end_z
                                && tp.elevation - bridge->floor_y <= 1;
                            if(!is_affected) { 
                                tp_i += 1;
                                continue; 
                            }
                            chunk.track_pieces.erase(
                                chunk.track_pieces.begin() + tp_i
                            );
                            this->world->terrain.reload_chunk_at(ch_x, ch_z);
                        }
                    }
                }
                u64 build_cost = bridge->length() * b_type.cost_per_tile;
                u64 refunded = (u64) ((f64) build_cost * demolition_refund_factor);
                size_t bridge_idx = bridge - this->world->terrain.bridges.data();
                this->world->terrain.bridges.erase(
                    this->world->terrain.bridges.begin() + bridge_idx
                );
                this->world->balance.add_coins(refunded, this->toasts);
                this->world->carriages.reset(&this->toasts);
                this->world->boats.reset(&this->toasts);
                this->selection.type = Selection::None;
                this->speaker.play(scene.get(sound::demolish));
                return;
            }
            case Selection::TrackPiece: {
                const Selection::TrackPieceSelection& tp_s
                    = this->selection.value.track_piece;
                Terrain::ChunkData& chunk = this->world->terrain
                    .chunk_at(tp_s.chunk_x, tp_s.chunk_z);
                const TrackPiece& removed_piece 
                    = chunk.track_pieces[tp_s.piece_i];
                auto removed_id 
                    = TrackPieceId(tp_s.chunk_x, tp_s.chunk_z, tp_s.piece_i);
                bool allowed = true;
                for(const Train& train: this->world->trains.agents) {
                    for(const Train::CarPosition& car_pos: train.cars) {
                        allowed &= car_pos.first.piece_id != removed_id;
                        allowed &= car_pos.second.piece_id != removed_id;
                    }
                }
                if(!allowed) {
                    this->toasts.add_error("toast_tracks_occupied", {});
                    return;
                }
                this->speaker.position 
                    = Vec<3>(tp_s.tile_x + 0.5, 0.0, tp_s.tile_z + 0.5)
                    * this->world->terrain.units_per_tile()
                    + Vec<3>(0, removed_piece.elevation, 0);
                chunk.track_pieces.erase(
                    chunk.track_pieces.begin() + tp_s.piece_i
                );
                this->world->balance
                    .add_coins(track_removal_refund, this->toasts);
                this->world->terrain
                    .reload_chunk_at(tp_s.chunk_x, tp_s.chunk_z);
                this->world->trains.reset(&this->toasts);
                this->selection.type = Selection::None;
                this->speaker.play(scene.get(sound::demolish));
                return;
            }
        }
    }

    void DemolitionMode::update(
        const engine::Window& window, engine::Scene& scene, 
        const Renderer& renderer
    ) {
        (void) scene;
        this->speaker.update();
        this->selection.type = Selection::None;
        auto [tile_x, tile_z] = this->world->terrain.find_selected_terrain_tile(
            window.cursor_pos_ndc(), renderer, Vec<3>(0.5, 0, 0.5)
        );
        u64 chunk_x = tile_x / this->world->terrain.tiles_per_chunk();
        u64 chunk_z = tile_z / this->world->terrain.tiles_per_chunk();
        Terrain::ChunkData& hover_chunk 
            = this->world->terrain.chunk_at(chunk_x, chunk_z);
        // check for selected brige
        const Bridge* hover_bridge = this->world->terrain
            .bridge_at((i64) tile_x, (i64) tile_z);
        if(hover_bridge != nullptr) {
            this->selection.type = Selection::Bridge;
            this->selection.value.bridge = hover_bridge;
        }
        // check for track pieces
        u64 rel_ch_x = tile_x % this->world->terrain.tiles_per_chunk();
        u64 rel_ch_z = tile_z % this->world->terrain.tiles_per_chunk();
        f64 closest_track_point_dist = INFINITY;
        for(size_t tp_i = 0; tp_i < hover_chunk.track_pieces.size(); tp_i += 1) {
            const TrackPiece& piece = hover_chunk.track_pieces[tp_i];
            if(piece.x != rel_ch_x || piece.z != rel_ch_z) { continue; }
            const TrackPiece::TypeInfo& piece_info = TrackPiece::types()
                .at((size_t) piece.type);
            Mat<4> t = piece.build_transform(
                chunk_x, chunk_z, 
                this->world->terrain.tiles_per_chunk(), 
                this->world->terrain.units_per_tile()
            );
            for(const Vec<3>& point_model: piece_info.points) {
                Vec<3> point_world = (t * point_model.with(1.0)).swizzle<3>("xyz");
                Vec<2> point_ndc = renderer.world_to_ndc(point_world);
                f64 dist = (window.cursor_pos_ndc() - point_ndc).len();
                if(dist > closest_track_point_dist) { continue; }
                closest_track_point_dist = dist;
                this->selection.type = Selection::TrackPiece;
                this->selection.value.track_piece = { 
                    tile_x, tile_z, chunk_x, chunk_z, tp_i 
                };
            }
        }
        // check for selected building
        u64 hover_building_ch_x, hover_building_ch_z;
        const Building* hover_building = this->world->terrain.building_at(
            (i64) tile_x, (i64) tile_z,
            &hover_building_ch_x, &hover_building_ch_z
        );
        if(hover_building != nullptr) {
            this->selection.type = Selection::Building;
            this->selection.value.building = {
                tile_x, tile_z,
                hover_building_ch_x, hover_building_ch_z,
                hover_building
            };
        }
        // do demolition
        bool attempted = this->selection.type != Selection::None
            && window.was_pressed(engine::Button::Left)
            && !this->ui.is_hovered_over();
        if(attempted) {
            this->attempt_demolition(scene);
        }
    }

    void DemolitionMode::render(
        const engine::Window& window, engine::Scene& scene, 
        Renderer& renderer
    ) {
        const engine::Texture& wireframe_texture 
            = scene.get(ActionMode::wireframe_error_texture);
        switch(this->selection.type) {
            case Selection::None: return;
            case Selection::Building: {
                const Selection::BuildingSelection& building 
                    = this->selection.value.building;
                const Building::TypeInfo& b_type 
                    = building.selected->get_type_info();
                Mat<4> transform = this->world->terrain.building_transform(
                    *building.selected, building.chunk_x, building.chunk_z
                );
                b_type.render_buildings(
                    window, scene, renderer,
                    std::array { transform },
                    &wireframe_texture,
                    engine::DepthTesting::Disabled
                );
                return;
            }
            case Selection::Bridge: {
                const Bridge* bridge = this->selection.value.bridge;
                const Bridge::TypeInfo& b_type = bridge->get_type_info();
                renderer.render(
                    scene.get(b_type.model), 
                    bridge->get_instances(this->world->terrain.units_per_tile()),
                    nullptr, 0.0,
                    engine::FaceCulling::Enabled,
                    engine::DepthTesting::Disabled, 
                    &wireframe_texture
                );
                return;
            }
            case Selection::TrackPiece: {
                const Selection::TrackPieceSelection& track_piece_s
                    = this->selection.value.track_piece;
                const Terrain::ChunkData& chunk = this->world->terrain
                    .chunk_at(track_piece_s.chunk_x, track_piece_s.chunk_z);
                const TrackPiece& track_piece 
                    = chunk.track_pieces[track_piece_s.piece_i];
                const engine::Model::LoadArgs& model = TrackPiece::types()
                    .at((size_t) track_piece.type).model;
                Mat<4> instance = track_piece.build_transform(
                    track_piece_s.chunk_x, track_piece_s.chunk_z,
                    this->world->terrain.tiles_per_chunk(),
                    this->world->terrain.units_per_tile()
                );
                renderer.render(
                    scene.get(model), std::array { instance }, nullptr, 0.0,
                    engine::FaceCulling::Enabled,
                    engine::DepthTesting::Disabled, 
                    &wireframe_texture
                );
                return;
            }
        }
    }
    
}