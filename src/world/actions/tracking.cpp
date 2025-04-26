
#include "common.hpp"
#include <algorithm>

namespace houseofatmos::world {

    namespace ui = houseofatmos::engine::ui;


    void TrackingMode::init_ui() {
        this->ui.with_element(ui::Element()
            .as_phantom()
            .with_pos(ui::null, ui::null)
            .with_size(ui::width::window, ui::height::window)
            .as_movable()
        );
        this->track_markers = &this->ui.root.children.back();
    }

    // If 'value' is 'Ascending', it means that the direction must point
    // TOWARDS 'previous'.
    // If 'value' is 'Descending', it means that the direction must point
    // AWAY FROM 'previous'.
    // If 'previous' has no value, or 'value' is 'Any', 
    // directly assign 'value' as the direction value.
    static void fill_track_directions(
        Terrain& terrain, const TrackNetwork& network,
        std::vector<TrackNetwork::NodeId>& encountered,
        TrackNetwork::NodeId current, 
        TrackPiece::Direction value, 
        std::optional<TrackNetwork::NodeId> previous = std::nullopt
    ) {
        bool skip = std::find(encountered.begin(), encountered.end(), current)
            != encountered.end();
        if(skip) { return; }
        encountered.push_back(current);
        const TrackNetwork::Node& n = network.graph.at(current);
        u64 t_x = n.chunk_x * terrain.tiles_per_chunk() + current->x;
        u64 t_z = n.chunk_z * terrain.tiles_per_chunk() + current->z;
        std::vector<TrackPiece*> pieces;
        terrain.track_pieces_at((i64) t_x, (i64) t_z, &pieces);
        if(pieces.size() != 1) { return; }
        TrackPiece* piece = pieces[0];
        if(!previous.has_value()) {
            piece->direction = value;
            if(n.connected_low.size() == 1) {
                fill_track_directions(
                    terrain, network, encountered, 
                    n.connected_low[0], value, current
                );
            }
            if(n.connected_high.size() == 1) {
                TrackPiece::Direction inv_val
                    = value == TrackPiece::Ascending? TrackPiece::Descending
                    : value == TrackPiece::Descending? TrackPiece::Ascending
                    : TrackPiece::Any;
                fill_track_directions(
                    terrain, network, encountered, 
                    n.connected_high[0], inv_val, current
                );
            }
            return;
        }
        bool prev_in_low = std::find(
            n.connected_low.begin(), n.connected_low.end(), *previous
        ) != n.connected_low.end();
        switch(value) {
            case TrackPiece::Any: piece->direction = value; break;
            case TrackPiece::Ascending:
                // make direction point towards previous
                piece->direction = prev_in_low? TrackPiece::Descending 
                    : TrackPiece::Ascending;
                break;
            case TrackPiece::Descending: 
                // make direction point away from previous
                piece->direction = prev_in_low? TrackPiece::Ascending
                    : TrackPiece::Descending;
                break;
        }
        for(TrackNetwork::NodeId connected: n.connected_low) {
            fill_track_directions(
                terrain, network, encountered, 
                connected, value, current
            );
        }
        for(TrackNetwork::NodeId connected: n.connected_high) {
            fill_track_directions(
                terrain, network, encountered, 
                connected, value, current
            );
        }
    }

    static const i64 track_marker_ch_rad = 1;
    static const f64 max_display_tile_dist = 3;

    void TrackingMode::update_track_markers(
        const engine::Window& window, const Renderer& renderer, 
        u64 tile_x, u64 tile_z, std::optional<Vec<3>>& closest_marker
    ) {
        i64 s_ch_x = (i64) tile_x / (i64) this->world->terrain.tiles_per_chunk()
            - track_marker_ch_rad;
        s_ch_x = std::max(s_ch_x, (i64) 0);
        u64 e_ch_x = s_ch_x + track_marker_ch_rad * 2;
        e_ch_x = std::min(e_ch_x, this->world->terrain.width_in_chunks() - 1);
        i64 s_ch_z = (i64) tile_z / (i64) this->world->terrain.tiles_per_chunk()
            - track_marker_ch_rad;
        s_ch_z = std::max(s_ch_z, (i64) 0);
        u64 e_ch_z = s_ch_z + track_marker_ch_rad * 2;
        e_ch_z = std::min(e_ch_z, this->world->terrain.height_in_chunks() - 1);
        std::vector<Vec<4>> markers;
        for(u64 ch_x = (u64) s_ch_x; ch_x <= e_ch_x; ch_x += 1) {
            for(u64 ch_z = (u64) s_ch_z; ch_z <= e_ch_z; ch_z += 1) {
                const Terrain::ChunkData& chunk = this->world->terrain
                    .chunk_at(ch_x, ch_z);
                for(const TrackPiece& track_piece: chunk.track_pieces) {
                    Mat<4> t = track_piece.build_transform(
                        ch_x, ch_z,
                        this->world->terrain.tiles_per_chunk(), 
                        this->world->terrain.units_per_tile()
                    );
                    const TrackPiece::TypeInfo& piece_type = TrackPiece::types()
                        .at((size_t) track_piece.type);
                    markers.push_back(t * piece_type.points[0].with(1.0));
                    markers.push_back(t * piece_type.points.back().with(1.0));
                }
            }
        }
        this->track_markers->children.clear();
        closest_marker = std::nullopt;
        f64 closest_dist = INFINITY;
        for(const Vec<4>& marker_pos: markers) {
            Vec<3> world = marker_pos.swizzle<3>("xyz");
            f64 units_per_tile = this->world->terrain.units_per_tile();
            Vec<3> tile_offset = Vec<3>(tile_x, 0, tile_z)
                - (world * Vec<3>(1, 0, 1) / units_per_tile);
            if(tile_offset.len() > max_display_tile_dist) { continue; }
            Vec<2> ndc = renderer.world_to_ndc(world);
            this->track_markers->children.push_back(ui::Element()
                .as_phantom()
                .with_pos(
                    ui::horiz::window_ndc(ndc.x()) - ui::unit * 4, 
                    ui::vert::window_ndc(ndc.y()) - ui::unit * 4
                )
                .with_size(ui::unit * 8, ui::unit * 8)
                .with_background(&ui_icon::terrain_vertex)
                .as_movable()
            );
            f64 cursor_dist = (window.cursor_pos_ndc() - ndc).len();
            if(cursor_dist < closest_dist) {
                closest_dist = cursor_dist;
                closest_marker = world;
            }
        }
        for(u64 ch_x = (u64) s_ch_x; ch_x <= e_ch_x; ch_x += 1) {
            for(u64 ch_z = (u64) s_ch_z; ch_z <= e_ch_z; ch_z += 1) {
                Terrain::ChunkData& chunk = this->world->terrain
                    .chunk_at(ch_x, ch_z);
                for(TrackPiece& piece: chunk.track_pieces) {
                    u64 t_x = ch_x * this->world->terrain.tiles_per_chunk()
                        + piece.x;
                    u64 t_z = ch_z * this->world->terrain.tiles_per_chunk()
                        + piece.z;
                    bool is_conflict = this->world->terrain
                        .track_pieces_at((i64) t_x, (i64) t_z) > 1;
                    if(is_conflict) {
                        piece.direction = TrackPiece::Any;
                        continue;
                    }
                    const TrackPiece::TypeInfo& info 
                        = TrackPiece::types().at((size_t) piece.type);
                    Mat<4> inst = piece.build_transform(
                        ch_x, ch_z, this->world->terrain.tiles_per_chunk(), 
                        this->world->terrain.units_per_tile()
                    );
                    Vec<3> low_w = (inst * info.points[0].with(1.0))
                        .swizzle<3>("xyz");
                    Vec<3> high_w = (inst * info.points.back().with(1.0))
                        .swizzle<3>("xyz");
                    Vec<3> world = (high_w - low_w) / 2 + low_w;
                    Vec<2> ndc = renderer.world_to_ndc(world);
                    const ui::Background* icon = &ui_icon::track_any_icon;
                    if(piece.direction != TrackPiece::Any) {
                        Vec<2> low = renderer.world_to_ndc(low_w);
                        Vec<2> high = renderer.world_to_ndc(high_w);
                        Vec<2> dir = piece.direction == TrackPiece::Ascending
                            ? high - low : low - high;
                        f64 angle_cross 
                            = ui_icon::track_dir_icon_first.x() * dir.y()
                            - ui_icon::track_dir_icon_first.y() * dir.x();
                        f64 angle = atan2(
                            angle_cross, ui_icon::track_dir_icon_first.dot(dir)
                        );
                        if(angle < 0) { angle += 2 * pi; }
                        u64 icon_i = (u64) round(angle / (pi / 4.0));
                        icon_i %= ui_icon::track_dir_icons.size();
                        icon = &ui_icon::track_dir_icons[icon_i];
                    }
                    this->track_markers->children.push_back(ui::Element()
                        .with_pos(
                            ui::horiz::window_ndc(ndc.x()) - ui::unit * 4,
                            ui::vert::window_ndc(ndc.y()) - ui::unit * 4
                        )
                        .with_size(ui::unit * 8, ui::unit * 8)
                        .with_background(icon)
                        .with_click_handler([this, piece = &piece]() {
                            TrackPiece::Direction d = piece->direction;
                            switch(piece->direction) {
                                case TrackPiece::Any: 
                                    d = TrackPiece::Descending; break;
                                case TrackPiece::Descending:
                                    d = TrackPiece::Ascending; break;
                                case TrackPiece::Ascending:
                                    d = TrackPiece::Any; break;
                            }
                            std::vector<TrackNetwork::NodeId> modified;
                            fill_track_directions(
                                this->world->terrain, 
                                this->world->trains.network, 
                                modified, piece, d
                            );
                            this->world->trains.find_paths(&this->toasts);
                        })
                        .as_movable()
                    );
                }
            }
        }
    }

    static const Vec<2> track_base_dir_ndc = Vec<2>(0, 1); // NDC; up the screen

    void TrackingMode::select_piece_disconnected(
        const engine::Window& window, const Renderer& renderer,
        u64 tile_x, u64 tile_z
    ) {
        this->preview_ch_x = tile_x / this->world->terrain.tiles_per_chunk();
        this->preview_ch_z = tile_z / this->world->terrain.tiles_per_chunk();
        this->preview_piece.x = tile_x % this->world->terrain.tiles_per_chunk();
        this->preview_piece.z = tile_z % this->world->terrain.tiles_per_chunk();
        this->preview_piece.elevation = this->world->terrain
            .elevation_at(tile_x, tile_z);
        f64 center_x = ((f64) tile_x + 0.5) 
            * (f64) this->world->terrain.units_per_tile();
        f64 center_z = ((f64) tile_z + 0.5)
            * (f64) this->world->terrain.units_per_tile();
        Vec<3> center = Vec<3>(
            center_x, (f64) this->preview_piece.elevation, center_z
        );
        Vec<2> center_ndc = renderer.world_to_ndc(center);
        Vec<2> to_cursor = (window.cursor_pos_ndc() - center_ndc).normalized();
        f64 dir_cross = track_base_dir_ndc.x() * to_cursor.y()
            - track_base_dir_ndc.y() * to_cursor.x();
        f64 angle_rad = atan2(dir_cross, track_base_dir_ndc.dot(to_cursor));
        if(angle_rad < 0.0) { angle_rad += pi; }
        f64 angle_rot = angle_rad / (2.0 * pi); // angle in number of rotations
        if(angle_rot >= 1.0/16 && angle_rot < 3.0/16) {
            this->preview_piece.type = TrackPiece::Diagonal;
            this->preview_piece.angle_q = 0;
        } else if(angle_rot >= 3.0/16 && angle_rot < 5.0/16) {
            this->preview_piece.type = TrackPiece::Straight;
            this->preview_piece.angle_q = 1;
        } else if(angle_rot >= 5.0/16 && angle_rot < 7.0/16) {
            this->preview_piece.type = TrackPiece::Diagonal;
            this->preview_piece.angle_q = 1;
        } else {
            this->preview_piece.type = TrackPiece::Straight;
            this->preview_piece.angle_q = 0;
        }
    }

    static const i16 max_piece_elev_d = 1;
    static const f64 max_piece_og_dist = 0.05;

    void TrackingMode::attempt_piece_connection(
        const engine::Window& window, const Renderer& renderer,
        u64 tx, u64 tz, u64 ch_x, u64 ch_z, i16 elev_d, size_t pt_i, i8 angle_q,
        f64& closest_cursor_dist
    ) {
        const auto& p_info = TrackPiece::types().at(pt_i);
        TrackPiece piece = TrackPiece(
            tx % this->world->terrain.tiles_per_chunk(), 
            tz % this->world->terrain.tiles_per_chunk(),
            (TrackPiece::Type) pt_i,
            angle_q, TrackPiece::Any, (i16) this->drag_origin->y() + elev_d
        );
        Mat<4> t = piece.build_transform(
            ch_x, ch_z, 
            this->world->terrain.tiles_per_chunk(), 
            this->world->terrain.units_per_tile()
        );
        Vec<3> a = (t * p_info.points[0].with(1.0))
            .swizzle<3>("xyz");
        f64 a_og_dist = (*this->drag_origin - a).len();
        Vec<3> b = (t * p_info.points.back().with(1.0))
            .swizzle<3>("xyz");
        f64 b_og_dist = (*this->drag_origin - b).len();
        Vec<3> o = a_og_dist < b_og_dist? a : b;
        if((o - *this->drag_origin).len() > max_piece_og_dist) { return; }
        Vec<3> c = a_og_dist < b_og_dist? b : a;
        Vec<2> c_ndc = renderer.world_to_ndc(c);
        f64 c_dist = (window.cursor_pos_ndc() - c_ndc).len();
        if(c_dist > closest_cursor_dist) { return; }
        closest_cursor_dist = c_dist;
        this->preview_piece = piece;
        this->preview_ch_x = ch_x;
        this->preview_ch_z = ch_z;
    }

    void TrackingMode::select_piece_connected(
        const engine::Window& window, const Renderer& renderer,
        Vec<3> closest_marker
    ) {
        bool started_dragging = !this->drag_origin.has_value()
            && window.is_down(engine::Button::Left)
            && !this->ui.is_hovered_over();
        if(started_dragging) { this->drag_origin = closest_marker; }
        bool stopped_dragging = this->drag_origin.has_value()
            && !window.is_down(engine::Button::Left);
        if(stopped_dragging) { this->drag_origin = std::nullopt; }
        if(!this->drag_origin.has_value()) { return; }
        const Terrain& terrain = this->world->terrain;
        u64 origin_tx = (u64) this->drag_origin->x() / terrain.units_per_tile();
        u64 origin_tz = (u64) this->drag_origin->z() / terrain.units_per_tile();
        u64 start_tx = origin_tx >= 1? origin_tx - 1 : origin_tx;
        u64 start_tz = origin_tz >= 1? origin_tz - 1 : origin_tz;
        u64 end_tx = std::min(origin_tx + 1, terrain.width_in_tiles() - 1);
        u64 end_tz = std::min(origin_tz + 1, terrain.height_in_tiles() - 1);
        size_t piece_type_c = TrackPiece::types().size();
        f64 closest_cursor_dist = INFINITY;
        for(u64 tx = start_tx; tx <= end_tx; tx += 1) {
            for(u64 tz = start_tz; tz <= end_tz; tz += 1) {
                u64 ch_x = tx / terrain.tiles_per_chunk();
                u64 ch_z = tz / terrain.tiles_per_chunk();
                i16 min_ed = -max_piece_elev_d;
                i16 max_ed = +max_piece_elev_d;
                for(i16 elev_d = min_ed; elev_d <= max_ed; elev_d += 1) {
                    for(size_t pt_i = 0; pt_i < piece_type_c; pt_i += 1) {
                        if(!TrackPiece::types().at(pt_i).has_ballast) {
                            continue;
                        }
                        for(i8 angle_q = 0; angle_q < 4; angle_q += 1) {
                            this->attempt_piece_connection(
                                window, renderer, tx, tz, ch_x, ch_z, elev_d, 
                                pt_i, angle_q, closest_cursor_dist
                            );
                        }
                    }
                }
            }
        }
    }

    void TrackingMode::determine_piece_valid(u64 tile_x, u64 tile_z) {
        const TrackPiece::TypeInfo& preview_piece_info = TrackPiece::types()
            .at((size_t) this->preview_piece.type);
        Mat<4> piece_instance = this->preview_piece.build_transform(
            this->preview_ch_x, this->preview_ch_z, 
            this->world->terrain.tiles_per_chunk(), 
            this->world->terrain.units_per_tile()
        );
        this->placement_valid = this->world->terrain
            .building_at((i64) tile_x, (i64) tile_z) == nullptr;
        std::vector<TrackPiece*> existing_pieces;
        this->world->terrain
            .track_pieces_at((i64) tile_x, (i64) tile_z, &existing_pieces);
        for(const TrackPiece* existing_piece: existing_pieces) {
            bool eq = existing_piece->type == this->preview_piece.type
                && existing_piece->x == this->preview_piece.x
                && existing_piece->z == this->preview_piece.z
                && existing_piece->angle_q == this->preview_piece.angle_q;
            this->placement_valid &= !eq;
        }
        for(const Vec<3>& model_point: preview_piece_info.points) {
            Vec<3> point = (piece_instance * model_point.with(1.0))
                .swizzle<3>("xyz");
            const Bridge* on_bridge = this->world->terrain
                .bridge_at((i64) tile_x, (i64) tile_z, point.y());
            if(on_bridge != nullptr) {
                if(preview_piece_info.ballastless.has_value()) {
                    this->preview_piece.type = *preview_piece_info.ballastless;
                }
                bool has_ballast = TrackPiece::types()
                    .at((size_t) this->preview_piece.type).has_ballast;
                this->placement_valid &= !has_ballast;
            }
            f64 elevation = this->world->terrain.elevation_at(point);
            this->placement_valid &= point.y() >= elevation;
            this->placement_valid &= point.y() - elevation <= 1;
        }
    }

    static const u64 track_placement_cost = 100;

    void TrackingMode::update(
        const engine::Window& window, engine::Scene& scene, 
        const Renderer& renderer
    ) {
        (void) scene;
        this->speaker.update();
        // determine the track we we want to place
        auto [tile_x, tile_z] = this->world->terrain.find_selected_terrain_tile(
            window.cursor_pos_ndc(), renderer, Vec<3>(0.5, 0, 0.5)
        );
        std::optional<Vec<3>> closest_marker;
        this->update_track_markers(
            window, renderer, tile_x, tile_z, closest_marker
        );
        if(closest_marker.has_value()) {
            this->select_piece_connected(window, renderer, *closest_marker);
        } else {
            this->select_piece_disconnected(window, renderer, tile_x, tile_z);
        }
        u64 dx = this->preview_ch_x * this->world->terrain.tiles_per_chunk()
            + this->preview_piece.x;
        u64 dz = this->preview_ch_z * this->world->terrain.tiles_per_chunk()
            + this->preview_piece.z;
        this->determine_piece_valid(dx, dz);
        bool place_track = this->permitted
            && this->placement_valid 
            && window.was_released(engine::Button::Left)
            && !this->ui.is_hovered_over()
            && this->world->balance.pay_coins(track_placement_cost, this->toasts);
        if(place_track) {
            Terrain::ChunkData& chunk = this->world->terrain
                .chunk_at(this->preview_ch_x, this->preview_ch_z);
            chunk.track_pieces.push_back(this->preview_piece);
            this->world->terrain.remove_foliage_at((i64) dx, (i64) dz);
            this->world->trains.find_paths(&this->toasts);
            this->speaker.position = Vec<3>(dx + 0.5, 0.0, dz + 0.5)
                * this->world->terrain.units_per_tile()
                + Vec<3>(0, this->preview_piece.elevation, 0);
            this->speaker.play(scene.get(sound::build));
        }
    }

    void TrackingMode::render(
        const engine::Window& window, engine::Scene& scene, 
        Renderer& renderer
    ) {
        (void) window;
        if(!this->permitted) { return; }
        const engine::Texture& wireframe_texture = this->placement_valid
            ? scene.get(ActionMode::wireframe_valid_texture)
            : scene.get(ActionMode::wireframe_error_texture);
        Mat<4> piece_instance = this->preview_piece.build_transform(
            this->preview_ch_x, this->preview_ch_z, 
            this->world->terrain.tiles_per_chunk(), 
            this->world->terrain.units_per_tile()
        );
        const engine::Model::LoadArgs& model = TrackPiece::types()
            .at((size_t) this->preview_piece.type).model;
        renderer.render(
            scene.get(model), 
            std::array { piece_instance }, nullptr, 0.0,
            engine::FaceCulling::Enabled,
            engine::DepthTesting::Enabled,
            &wireframe_texture
        );
    }
    
}