
#include "train.hpp"
#include "../particle_const.hpp"
#include <unordered_set>

namespace houseofatmos::world {

    static const f64 max_piece_point_dist = 0.001;

    static bool track_piece_connected_to(
        std::span<const Vec<3>> pc_pts, const Mat<4>& pc_inst,
        const Vec<3>& to_pt, const Mat<4>& to_inst 
    ) {
        Vec<3> pc_low = (pc_inst * pc_pts[0].with(1)).swizzle<3>("xyz");
        Vec<3> pc_high = (pc_inst * pc_pts.back().with(1)).swizzle<3>("xyz");
        Vec<3> to = (to_inst * to_pt.with(1.0)).swizzle<3>("xyz");
        return (pc_low  - to).len() <= max_piece_point_dist
            || (pc_high - to).len() <= max_piece_point_dist;
    }

    static const f64 max_piece_dir_diff = 0.001;

    static bool track_piece_connections_align(
        const Terrain& terrain,
        u64 pc_tx, u64 pc_tz, i16 pc_elev,
        std::span<const Vec<3>> pc_pts, const Mat<4>& pc_inst,
        u64 to_tx, u64 to_tz, i16 to_elev,
        const Vec<3>& to_pt, const Mat<4>& to_inst
    ) {
        Vec<3> pc_center = Vec<3>(pc_tx + 0.5, 0, pc_tz + 0.5)
            * terrain.units_per_tile()
            + Vec<3>(0, pc_elev, 0);
        Vec<3> pc_low = (pc_inst * pc_pts[0].with(1)).swizzle<3>("xyz");
        Vec<3> pc_high = (pc_inst * pc_pts.back().with(1)).swizzle<3>("xyz");
        Vec<3> to_center = Vec<3>(to_tx + 0.5, 0, to_tz + 0.5)
            * terrain.units_per_tile()
            + Vec<3>(0, to_elev, 0);
        Vec<3> to = (to_inst * to_pt.with(1.0)).swizzle<3>("xyz");
        Vec<3> pc_to_low = pc_low - pc_center;
        Vec<3> pc_to_high = pc_high - pc_center;
        Vec<3> c_to_center = to - to_center;
        return (pc_to_low + c_to_center).len() <= max_piece_dir_diff
            || (pc_to_high + c_to_center).len() <= max_piece_dir_diff;
    }

    void TrackNetwork::find_connections(NodeId node_id, Node& node_data) {
        const TrackPiece& node = this->track_piece_at(node_id);
        u64 tiles_per_chunk = this->terrain->tiles_per_chunk();
        u64 world_wch = this->terrain->width_in_chunks();
        u64 world_hch = this->terrain->height_in_chunks();
        const std::vector<Vec<3>>& node_pts = TrackPiece::types()
            .at((size_t) node.type).points;
        Mat<4> node_inst = node.build_transform(
            node_id.chunk_x, node_id.chunk_z, tiles_per_chunk,
            this->terrain->units_per_tile()
        );
        u64 node_tx = node_id.chunk_x * tiles_per_chunk + node.x;
        u64 node_tz = node_id.chunk_z * tiles_per_chunk + node.z;
        u64 min_chx = (node_tx > 0? node_tx - 1 : 0) / tiles_per_chunk;
        u64 min_chz = (node_tz > 0? node_tz - 1 : 0) / tiles_per_chunk;
        u64 max_chx = std::min((node_tx + 1) / tiles_per_chunk, world_wch - 1);
        u64 max_chz = std::min((node_tz + 1) / tiles_per_chunk, world_hch - 1);
        for(u64 conn_chx = min_chx; conn_chx <= max_chx; conn_chx += 1) {
            for(u64 conn_chz = min_chz; conn_chz <= max_chz + 1; conn_chz += 1) {
                const Terrain::ChunkData& conn_chunk = this->terrain
                    ->chunk_at(conn_chx, conn_chz);
                for(size_t pc_i = 0; pc_i < conn_chunk.track_pieces.size(); pc_i += 1) {
                    const TrackPiece& conn_piece = conn_chunk.track_pieces[pc_i];
                    u64 pc_tx = conn_chx * tiles_per_chunk + conn_piece.x;
                    u64 pc_tz = conn_chz * tiles_per_chunk + conn_piece.z;
                    if(node_tx == pc_tx && node_tz == pc_tz) { continue; }
                    const std::vector<Vec<3>>& conn_pts = TrackPiece::types()
                        .at((size_t) conn_piece.type).points;
                    Mat<4> conn_inst = conn_piece.build_transform(
                        conn_chx, conn_chz, tiles_per_chunk, 
                        this->terrain->units_per_tile()
                    );
                    bool connected_low = track_piece_connected_to(
                        conn_pts, conn_inst, node_pts[0], node_inst
                    ) && track_piece_connections_align(
                        *this->terrain, 
                        pc_tx, pc_tz, conn_piece.elevation, 
                        conn_pts, conn_inst,
                        node_tx, node_tz, node.elevation, 
                        node_pts[0], node_inst
                    );
                    if(connected_low) { 
                        node_data.connected_low.push_back(
                            TrackPieceId(conn_chx, conn_chz, pc_i)
                        ); 
                    }
                    bool connected_high = track_piece_connected_to(
                        conn_pts, conn_inst, node_pts.back(), node_inst
                    ) && track_piece_connections_align(
                        *this->terrain, 
                        pc_tx, pc_tz, conn_piece.elevation, 
                        conn_pts, conn_inst,
                        node_tx, node_tz, node.elevation, 
                        node_pts.back(), node_inst
                    );
                    if(connected_high) {
                        node_data.connected_high.push_back(
                            TrackPieceId(conn_chx, conn_chz, pc_i)
                        );
                    }
                }
            }
        }
    }

    static const u64 max_block_size = 4;

    void TrackNetwork::assign_nodes_to_blocks(
        TileNetwork::NodeId tile, Block* previous
    ) {
        auto [tx, tz] = tile;
        u64 cx = tx / this->terrain->tiles_per_chunk();
        u64 cz = tz / this->terrain->tiles_per_chunk();
        u64 crx = tx - (cx * this->terrain->tiles_per_chunk());
        u64 crz = tz - (cz * this->terrain->tiles_per_chunk());
        const Terrain::ChunkData& chunk = this->terrain->chunk_at(cx, cz);
        std::vector<TrackPieceId> pieces;
        for(size_t pc_i = 0; pc_i < chunk.track_pieces.size(); pc_i += 1) {
            const TrackPiece& piece = chunk.track_pieces[pc_i];
            if(piece.x != crx || piece.z != crz) { continue; }
            pieces.push_back(TrackPieceId(cx, cz, pc_i));
        }
        if(pieces.size() == 0) { return; }
        bool is_conflict = pieces.size() > 1;
        for(const TrackPieceId& piece_id: pieces) {
            const TrackPiece& piece = this->track_piece_at(piece_id);
            if(this->graph.at(piece_id).block != nullptr) { return; }
            is_conflict |= piece.direction == TrackPiece::Any;
        }
        bool add_to_prev = !is_conflict
            && previous != nullptr
            && previous->type != Block::Conflict
            && previous->size < max_block_size;
        Block* block = previous;
        if(!add_to_prev) {
            Block::Type type = is_conflict? Block::Conflict : Block::Simple;
            this->blocks.push_back(Block(type));
            block = &this->blocks.back();
        }
        block->size += 1;
        u64 tpc = this->terrain->tiles_per_chunk();
        for(const TrackPieceId& piece_id: pieces) {
            Node& node = this->graph.at(piece_id);
            // this assignment doesn't need to be in a separate loop
            // since the above checks stop if ANY track piece on the
            // same tile already has an assigned block
            node.block = block;
            for(NodeId conn: node.connected_low) {
                const TrackPiece& conn_piece = this->track_piece_at(piece_id);
                u64 ctx = conn.chunk_x * tpc + conn_piece.x;
                u64 ctz = conn.chunk_z * tpc + conn_piece.z;
                this->assign_nodes_to_blocks({ ctx, ctz }, node.block);
            }
            for(NodeId conn: node.connected_high) {
                const TrackPiece& conn_piece = this->track_piece_at(piece_id);
                u64 ctx = conn.chunk_x * tpc + conn_piece.x;
                u64 ctz = conn.chunk_z * tpc + conn_piece.z;
                this->assign_nodes_to_blocks({ ctx, ctz }, node.block);
            }
        }
    }

    static const f64 signal_track_dist = 2.0;
    static const Vec<3> signal_model_dir = Vec<3>(0, 0, 1);

    void TrackNetwork::create_signals(NodeId node_i) {
        const Node& node = this->graph.at(node_i);
        const TrackPiece& piece = this->track_piece_at(node_i);
        const std::vector<NodeId>* connected = nullptr;
        switch(piece.direction) {
            case TrackPiece::Any: return; // conflict
            case TrackPiece::Ascending: connected = &node.connected_high; break;
            case TrackPiece::Descending: connected = &node.connected_low; break;
            default: engine::error("Unhandled 'TrackPiece::Direction'!");
        }
        u64 tiles_per_chunk = this->terrain->tiles_per_chunk();
        u64 units_per_tile = this->terrain->units_per_tile();
        const std::vector<Vec<3>>& piece_points = TrackPiece::types()
            .at((size_t) piece.type).points;
        Mat<4> inst = piece.build_transform(
            node_i.chunk_x, node_i.chunk_z, tiles_per_chunk, units_per_tile
        );
        Vec<3> low = (inst * piece_points[0].with(1)).swizzle<3>("xyz");
        Vec<3> high = (inst * piece_points.back().with(1)).swizzle<3>("xyz");
        Vec<3> pos = (high - low) / 2 + low;
        for(NodeId c_node_i: *connected) {
            const Node& c_node = this->graph.at(c_node_i);
            const TrackPiece& c_piece = this->track_piece_at(c_node_i);
            if(c_node.block == node.block) { continue; }
            u64 c_tx = node_i.chunk_x * tiles_per_chunk + piece.x;
            u64 c_tz = node_i.chunk_z * tiles_per_chunk + piece.z;
            Vec<3> c_pos = Vec<3>(c_tx + 0.5, 0, c_tz + 0.5) * units_per_tile
                + Vec<3>(0, c_piece.elevation, 0);
            Vec<3> dir = c_pos - pos;
            Vec<3> offset = Vec<3>(dir.z(), dir.y(), -dir.x()).normalized();
            if(!this->settings->signal_side_left) { offset *= -1; }
            Vec<3> signal_pos = pos + offset * signal_track_dist;
            f64 angle_cross = signal_model_dir.x() * dir.z()
                - signal_model_dir.z() * dir.x();
            f64 signal_angle = atan2(angle_cross, signal_model_dir.dot(dir));
            this->signals.push_back(Signal(
                node.block, c_node.block, signal_pos, signal_angle
            ));
        }
    }

    void TrackNetwork::reset() {
        this->graph.clear();
        u64 world_w_ch = this->terrain->width_in_chunks();
        u64 world_h_ch = this->terrain->height_in_chunks();
        for(u64 chunk_x = 0; chunk_x < world_w_ch; chunk_x += 1) {
            for(u64 chunk_z = 0; chunk_z < world_h_ch; chunk_z += 1) {
                const Terrain::ChunkData& chunk = this->terrain
                    ->chunk_at(chunk_x, chunk_z);
                for(size_t pc_i = 0; pc_i < chunk.track_pieces.size(); pc_i += 1) {
                    auto node = Node({}, {});
                    auto node_id = TrackPieceId(chunk_x, chunk_z, pc_i);
                    this->find_connections(node_id, node);
                    this->graph[node_id] = node;
                }
            }
        }
        u64 tpc = this->terrain->tiles_per_chunk();
        this->blocks.clear();
        for(u64 chunk_x = 0; chunk_x < world_w_ch; chunk_x += 1) {
            for(u64 chunk_z = 0; chunk_z < world_h_ch; chunk_z += 1) {
                const Terrain::ChunkData& chunk = this->terrain
                    ->chunk_at(chunk_x, chunk_z);
                for(const TrackPiece& track_piece: chunk.track_pieces) {
                    u64 ptx = chunk_x * tpc + track_piece.x;
                    u64 ptz = chunk_z * tpc + track_piece.z;
                    this->assign_nodes_to_blocks({ ptx, ptz });
                }
            }
        }
        this->signals.clear();
        std::unordered_set<TileNetworkNode::NodeId, TileNetworkNode::NodeIdHash>
            signalled_tiles;
        for(u64 chunk_x = 0; chunk_x < world_w_ch; chunk_x += 1) {
            for(u64 chunk_z = 0; chunk_z < world_h_ch; chunk_z += 1) {
                const Terrain::ChunkData& chunk = this->terrain
                    ->chunk_at(chunk_x, chunk_z);
                for(size_t pc_i = 0; pc_i < chunk.track_pieces.size(); pc_i += 1) {
                    const TrackPiece& track_piece = chunk.track_pieces[pc_i];
                    u64 ptx = chunk_x * tpc + track_piece.x;
                    u64 ptz = chunk_z * tpc + track_piece.z;
                    TileNetworkNode::NodeId tile = { ptx, ptz };
                    if(signalled_tiles.contains(tile)) { continue; }
                    this->create_signals(TrackPieceId(chunk_x, chunk_z, pc_i));
                    signalled_tiles.insert(tile);
                }
            }
        }
    }


    void TrackNetwork::collect_next_nodes(
        std::optional<NodeId> prev, NodeId node_i, 
        std::vector<std::pair<NodeId, u64>>& out
    ) {
        Node node = this->graph[node_i];
        const TrackPiece& piece = this->track_piece_at(node_i);
        const std::vector<NodeId>& low = node.connected_low;
        const std::vector<NodeId>& high = node.connected_high;
        // previous piece is connected at LOW end of this piece?
        // -> connects to all pieces at HIGH end
        bool prev_at_low = !prev.has_value()
            || std::find(low.begin(), low.end(), *prev) != low.end();
        bool is_ascending = piece.direction == TrackPiece::Ascending
            || piece.direction == TrackPiece::Any;
        if(prev_at_low && is_ascending) {
            for(NodeId c: high) { out.push_back({ c, 1 }); }
        }
        // previous piece is connected at HIGH end of this piece?
        // -> connects to all pieces at LOW end
        bool prev_at_high = !prev.has_value()
            || std::find(high.begin(), high.end(), *prev) != high.end();
        bool is_descending = piece.direction == TrackPiece::Descending
            || piece.direction == TrackPiece::Any;
        if(prev_at_high && is_descending) {
            for(NodeId c: low) { out.push_back({ c, 1 }); }
        }
    }

    u64 TrackNetwork::node_target_dist(NodeId node_i, ComplexId target) {
        // find node position
        const TrackPiece& piece = this->track_piece_at(node_i);
        u64 nx = node_i.chunk_x * this->terrain->tiles_per_chunk() + piece.x;
        u64 nz = node_i.chunk_z * this->terrain->tiles_per_chunk() + piece.z;
        // find target building start coords
        const Complex& complex = this->complexes->get(target);
        auto [bsx, bsz] = complex.closest_member_to(nx, nz);
        // find target building end coordinates
        // (end in this case meaning the last tile still inside the building)
        const Building* building = this->terrain
            ->building_at((i64) bsx, (i64) bsz);
        const Building::TypeInfo& building_type = Building::types()
            .at((size_t) building->type);
        u64 bex = bsx + building_type.width - 1;
        u64 bez = bsz + building_type.height - 1;
        // distance on each individual axis
        u64 dx = nx < bsx? bsx - nx // left of building
            : nx > bex? nx - bex    // right of building
            : 0;                    // on X axis inside building
        u64 dz = nz < bsz? bsz - nz // top of building
            : nz > bez? nz - bez    // below building
            : 0;                    // on Z axis inside building
        return dx + dz; // manhattan distance
    }

    bool TrackNetwork::node_at_target(NodeId node, ComplexId target) {
        return this->node_target_dist(node, target) <= 2;
    }

    void TrackNetwork::update(
        engine::Scene& scene, const engine::Window& window
    ) {
        (void) scene;
        for(Signal& signal: this->signals) {
            signal.update(window);
        }
    }

    void TrackNetwork::render(
        const Vec<3>& observer, f64 draw_distance,
        Renderer& renderer, engine::Scene& scene, 
        const engine::Window& window
    ) {
        (void) window;
        for(Signal& signal: this->signals) {
            f64 distance = (signal.position - observer).len();
            if(distance > draw_distance) { continue; }
            signal.render(*this, renderer, scene);
        }
    }



    Vec<3> TrackPosition::in_world(const TrackNetwork& network) const {
        const TrackPiece& piece = network.track_piece_at(this->piece_id);
        const TrackPiece::TypeInfo& pc_info 
            = TrackPiece::types().at((size_t) piece.type);
        if(pc_info.points.size() == 0) {
            engine::error("Illegal piece type (points.size() == 0)");
        }
        f64 remaining = this->direction == TrackPiece::Direction::Ascending
            ? this->distance : this->remaining(network);
        Vec<3> point = pc_info.points.back();
        for(size_t i = 1; i < pc_info.points.size(); i += 1) {
            Vec<3> prev = pc_info.points[i - 1];
            Vec<3> next = pc_info.points[i];
            Vec<3> step = next - prev;
            f64 step_len = step.len();
            if(remaining > step_len) {
                remaining -= step_len;
                continue;
            }
            f64 step_prog = remaining / step_len;
            point = prev + (step * step_prog);
            break;
        }
        Mat<4> t = piece.build_transform(
            this->piece_id.chunk_x, this->piece_id.chunk_z, 
            network.terrain->tiles_per_chunk(), 
            network.terrain->units_per_tile()
        );
        return (t * point.with(1.0)).swizzle<3>("xyz");
    }

    TrackPosition TrackPosition::move_along(
        const AgentPath<TrackNetwork>& path, const TrackNetwork& network,
        f64 distance, bool* at_end_out
    ) const {
        size_t og_pt_i = SIZE_MAX;
        for(size_t p = 0; p < path.points.size(); p += 1) {
            if(path.points[p] != this->piece_id) { continue; }
            og_pt_i = p;
            break;
        }
        if(og_pt_i == SIZE_MAX || distance == 0.0) { return *this; }
        bool is_backwards = distance < 0;
        size_t pt_i = og_pt_i;
        auto pos = *this;
        pos.distance += distance;
        for(;;) {
            TrackPieceId prev = pos.piece_id;
            const TrackPiece& prev_piece = network.track_piece_at(prev);
            f64 prev_piece_len = TrackPiece::types()
                .at((size_t) prev_piece.type).length();
            if(is_backwards) {
                if(pos.distance >= 0.0) { break; }
                if(pt_i == 0) {
                    if(at_end_out != nullptr) { *at_end_out = true; }
                    pos.distance = 0.0;
                    break;
                }
                pt_i -= 1;
            } else {
                if(prev_piece_len > pos.distance) { break; }
                pt_i += 1;
                if(pt_i >= path.points.size()) {
                    if(at_end_out != nullptr) { *at_end_out = true; }
                    pos.distance = prev_piece_len;
                    break;
                }
            }
            pos.piece_id = path.points[pt_i];
            if(is_backwards) {
                pos.distance += prev_piece_len;
            } else {
                pos.distance -= prev_piece_len;
            }
            const TrackNetwork::Node& next_node 
                = network.graph.at(pos.piece_id);
            bool is_ascending = std::find(
                next_node.connected_low.begin(), 
                next_node.connected_low.end(),
                prev
            ) != next_node.connected_low.end();
            if(is_backwards) { is_ascending = !is_ascending; }
            pos.direction = is_ascending
                ? TrackPiece::Direction::Ascending 
                : TrackPiece::Direction::Descending;
        }
        return pos;
    }



    static Train::Car train_car_old = Train::Car(
        engine::Model::LoadArgs(
            "res/trains/train_car_old.glb", Renderer::model_attribs,
            engine::FaceCulling::Enabled
        ),
        Vec<3>(0, 0, 1), // model heading
        3.6, // length
        2.0, // axle length
        0.5, // wheel radius
        150 // item capacity
    );

    static Train::Car train_car_modern = Train::Car(
        engine::Model::LoadArgs(
            "res/trains/train_car_modern.glb", Renderer::model_attribs,
            engine::FaceCulling::Enabled
        ),
        Vec<3>(0, 0, 1), // model heading
        4.0, // length
        2.4, // axle length
        0.5, // wheel radius
        250 // item capacity
    );

    static Train::Car train_car_tram = Train::Car(
        engine::Model::LoadArgs(
            "res/trains/tram_car.glb", 
            Renderer::model_attribs,
            engine::FaceCulling::Disabled
        ),
        Vec<3>(0, 0, 1), // model heading
        5.0, // length
        2.8, // axle length
        0.5, // wheel radius
        0 // item capacity
    );

    static std::vector<Train::LocomotiveTypeInfo> locomotive_infos = {
        /* Basic */ {
            "locomotive_name_basic",
            &ui_icon::basic_locomotive,
            research::Research::Reward::BasicLocomotive,
            {
                Train::Car(
                    engine::Model::LoadArgs(
                        "res/trains/basic_locomotive.glb", 
                        Renderer::model_attribs,
                        engine::FaceCulling::Disabled
                    ),
                    Vec<3>(0, 0, 1), // model heading
                    3.62, // length
                    1.1, // axle length
                    0.9, // wheel radius (of big wheel)
                    50 // item capacity
                )
            },
            train_car_old,
            {
                (Train::LocomotiveTypeInfo::Driver) {
                    Vec<3>(0.5, 0.9, -1.72), // position offset
                    0.0, // rotation offset
                    (u64) human::Animation::Stand // animation
                }
            },
            Vec<3>(0.0, 3.0, 1.22), // relative smoke origin
            1.4, // whistle pitch
            3, // max car count
            2.0, // acceleration
            1.75, // braking distance
            5.0, // top speed
            5000, // cost
            Player::Rideable(
                Vec<3>(-0.5, 0.9, -1.72), // position offset
                0.0, // rotation offset
                (u64) human::Animation::Stand // animation
            )
        },
        /* Small */ {
            "locomotive_name_small",
            &ui_icon::small_locomotive,
            research::Research::Reward::SmallLocomotive,
            {
                Train::Car(
                    engine::Model::LoadArgs(
                        "res/trains/small_locomotive.glb", 
                        Renderer::model_attribs,
                        engine::FaceCulling::Disabled
                    ),
                    Vec<3>(0, 0, 1), // model heading
                    5.0, // length
                    1.8, // axle length
                    0.9, // wheel radius (of big wheels)
                    50 // item capacity
                )
            },
            train_car_old,
            {
                (Train::LocomotiveTypeInfo::Driver) {
                    Vec<3>(0.5, 1.325, -2.20), // position offset
                    0.0, // rotation offset
                    (u64) human::Animation::Sit // animation
                }
            },
            Vec<3>(0.0, 2.6, 2.0), // relative smoke origin
            1.3, // whistle pitch
            4, // max car count
            1.75, // acceleration
            2.0, // braking distance
            7.5, // top speed
            7500, // cost
            Player::Rideable(
                Vec<3>(-0.5, 1.325, -2.20), // position offset
                0.0, // rotation offset
                (u64) human::Animation::Sit // animation
            )
        },
        /* Tram */ {
            "locomotive_name_tram",
            &ui_icon::tram,
            research::Research::Reward::Tram,
            {
                Train::Car(
                    engine::Model::LoadArgs(
                        "res/trains/tram_locomotive.glb", 
                        Renderer::model_attribs,
                        engine::FaceCulling::Disabled
                    ),
                    Vec<3>(0, 0, 1), // model heading
                    5.0, // length
                    2.8, // axle length
                    0.5, // wheel radius
                    25 // item capacity
                )
            },
            train_car_tram,
            {
                (Train::LocomotiveTypeInfo::Driver) {
                    Vec<3>(0.0, 1.2, 1.6), // position offset
                    0.0, // rotation offset
                    (u64) human::Animation::Sit // animation
                }
            },
            Vec<3>(0.395, 3.087, -1.702), // relative smoke origin
            1.35, // whistle pitch
            2, // max car count
            2.0, // acceleration
            1.5, // braking distance
            12.5, // top speed
            6500, // cost
            Player::Rideable(
                Vec<3>(0.0, 1.2, -2.15), // position offset
                0.0, // rotation offset
                (u64) human::Animation::Sit // animation
            )
        }
    };

    const std::vector<Train::LocomotiveTypeInfo>& Train::locomotive_types() {
        return locomotive_infos;
    }


    Train::Train(
        LocomotiveType loco_type, std::vector<CarPosition> cars, 
        const Settings& settings
    ): Agent<TrackNetwork>() {
        this->loco_type = loco_type;
        this->cars = cars;
        this->speaker.volume = settings.sfx_volume;
    }

    Train::Train(
        const Serialized& serialized, const engine::Arena& buffer,
        const Settings& settings
    ): 
        Agent<TrackNetwork>(serialized.agent, buffer) {
        this->loco_type = serialized.locomotive;
        buffer.copy_into(serialized.cars, this->cars);
        this->velocity = serialized.velocity;
        this->speaker.volume = settings.sfx_volume;
    }

    Train::Serialized Train::serialize(engine::Arena& buffer) const {
        return Serialized(
            Agent<TrackNetwork>::serialize(buffer), // this is a non-static
            this->loco_type,
            buffer.alloc(this->cars),
            this->velocity 
        );
    }

    f64 Train::wait_point_distance(TrackNetwork& network) const {
        if(!this->current_path().has_value()) { return 0.0; }
        const AgentPath<TrackNetwork>& path = *this->current_path();
        TrackPieceId f_piece = this->cars.front().first.piece_id;
        size_t front_point_i = SIZE_MAX;
        for(size_t p = 0; p < path.points.size(); p += 1) {
            TrackPieceId s_piece = path.points[p];
            if(s_piece != f_piece) { continue; }
            front_point_i = p;
            break;
        }
        if(front_point_i == SIZE_MAX) {
            engine::warning("Train broken! Current front segment is not in path");
            return 0.0;
        }
        f64 distance = this->cars.front().first.remaining(network);
        for(size_t p = front_point_i + 1; p < path.points.size(); p += 1) {
            TrackPieceId piece_id = path.points[p];
            const TrackNetwork::Node& node = network.graph.at(piece_id);
            if(node.block->owner != this) { 
                distance -= 2.5; // wait a bit before the next block
                break; 
            }
            const TrackPiece& piece = network.track_piece_at(piece_id);
            const TrackPiece::TypeInfo& pc_info 
                = TrackPiece::types().at((size_t) piece.type);
            distance += pc_info.length();
        }
        return distance;
    }

    Mat<4> Train::build_car_transform(
        const TrackNetwork& network,
        size_t car_idx, Vec<3>* position_out, f64* pitch_out, f64* yaw_out
    ) const {
        const Train::Car& car_info = this->car_at(car_idx);
        const Train::CarPosition& car_pos = this->cars[car_idx];
        Vec<3> front = car_pos.first.in_world(network);
        Vec<3> back = car_pos.second.in_world(network);
        Vec<3> position = (front - back) / 2.0 + back;
        Vec<3> heading = (front - back).normalized();
        auto [pitch, yaw] = Agent<TrackNetwork>
            ::compute_heading_angles(heading, car_info.model_heading);
        if(position_out != nullptr) { *position_out = position; }
        if(pitch_out != nullptr) { *pitch_out = pitch; }
        if(yaw_out != nullptr) { *yaw_out = yaw; }
        return Mat<4>::translate(position)
            * Mat<4>::rotate_y(yaw) 
            * Mat<4>::rotate_x(pitch);
    }

    void Train::release_unjustified_blocks(TrackNetwork& network) {
        if(!this->current_path().has_value()) { return; }
        const AgentPath<TrackNetwork>& path = *this->current_path();
        for(OwnedBlock& owning: this->owning_blocks) {
            owning.justified = false;
        }
        TrackPieceId f_piece = this->cars.front().first.piece_id;
        TrackPieceId b_piece = this->cars.back().second.piece_id;
        size_t front_point_i = SIZE_MAX;
        size_t back_point_i = SIZE_MAX;
        for(size_t p = 0; p < path.points.size(); p += 1) {
            TrackPieceId s_piece = path.points[p];
            if(s_piece == f_piece) { front_point_i = p; }
            if(b_piece == f_piece) { back_point_i = p; }
        }
        if(front_point_i == SIZE_MAX || back_point_i == SIZE_MAX) {
            engine::warning("Train broken! Current segments is not in path");
            return;
        }
        for(size_t p = back_point_i; p < front_point_i; p += 1) {
            TrackPieceId piece_id = path.points[p];
            const TrackNetwork::Node& node = network.graph.at(piece_id);
            if(node.block->owner != this) { continue; }
            auto owned_block = std::find_if(
                this->owning_blocks.begin(), this->owning_blocks.end(), 
                [n = &node](const auto& b) { return b.block == n->block; }
            );
            if(owned_block == this->owning_blocks.end()) { continue; }
            owned_block->justified = true;
        }
        for(OwnedBlock& owning: this->owning_blocks) {
            if(owning.justified) { continue; }
            owning.block->owner = nullptr;
        }
        auto new_owned_end = std::remove_if(
            this->owning_blocks.begin(), this->owning_blocks.end(),
            [](const auto& b) { return !b.justified; }
        );
        this->owning_blocks.erase(new_owned_end, this->owning_blocks.end());
    }

    void Train::take_next_blocks(TrackNetwork& network) {
        if(!this->current_path().has_value()) { return; }
        const AgentPath<TrackNetwork>& path = *this->current_path();
        TrackPieceId f_piece = this->cars.front().first.piece_id;
        size_t front_point_i = SIZE_MAX;
        for(size_t p = 0; p < path.points.size(); p += 1) {
            TrackPieceId s_piece = path.points[p];
            if(s_piece != f_piece) { continue; }
            front_point_i = p; 
            break;
        }
        if(front_point_i == SIZE_MAX) {
            engine::warning("Train broken! Current segments is not in path");
            return;
        }
        TrackNetwork::Block* current_block = network.graph
            .at(path.points[front_point_i]).block;
        if(current_block->owner != this) { current_block = nullptr; }
        bool may_take_all = true;
        for(size_t p = front_point_i + 1; p < path.points.size(); p += 1) {
            TrackPieceId piece_id = path.points[p];
            const TrackNetwork::Node& node = network.graph.at(piece_id);
            if(node.block == current_block) { continue; }
            may_take_all &= node.block->owner == nullptr;
            if(node.block->type == TrackNetwork::Block::Conflict) { continue; }
            break;
        }
        if(!may_take_all) { return; }
        for(size_t p = front_point_i + 1; p < path.points.size(); p += 1) {
            TrackPieceId piece_id = path.points[p];
            const TrackNetwork::Node& node = network.graph.at(piece_id);
            if(node.block == current_block) { continue; }
            node.block->owner = this;
            this->owning_blocks.push_back(OwnedBlock(node.block, true));
            if(node.block->type == TrackNetwork::Block::Conflict) { continue; }
            break;
        }
    }

    std::optional<AgentPath<TrackNetwork>> Train::find_path_to(
        TrackNetwork& network, ComplexId target
    ) {
        TrackPosition f_pos = this->cars.front().first;
        TrackPosition b_pos = this->cars.back().second;
        const TrackNetwork::Node& f_node = network.graph.at(f_pos.piece_id);
        auto path = AgentPath<TrackNetwork>::find(
            network, b_pos.piece_id, target,
            // banned tiles (stops reversal of train)
            f_pos.direction == TrackPiece::Direction::Ascending
                ? f_node.connected_low : f_node.connected_high
        );
        if(!path.has_value()) { return path; }
        // the generated path begins from the start of the train,
        // but it actually must start from the END of the train
        // for the rest of the functions to work correctly
        for(const CarPosition& car_pos: this->cars) { // front -> back of train
            TrackPieceId front = car_pos.first.piece_id;
            if(path->points.front() != front) {
                path->points.insert(path->points.begin(), front);
            }
            TrackPieceId back = car_pos.second.piece_id;
            if(path->points.front() != back) {
                path->points.insert(path->points.begin(), back);
            }
        }
        return path;
    }

    void Train::on_network_reset(TrackNetwork& network) {
        (void) network;
        this->owning_blocks.clear();
    }

    void Train::update_velocity(
        const engine::Window& window, TrackNetwork& network
    ) {
        const Train::LocomotiveTypeInfo& loco_info = Train::locomotive_types()
            .at((size_t) this->loco_type);
        f64 end_after = this->wait_point_distance(network);
        f64 stop_limit = end_after / loco_info.braking_distance;
        this->velocity += loco_info.acceleration * window.delta_time();
        this->velocity = std::max(this->velocity, 0.0);
        this->velocity = std::min(this->velocity, loco_info.top_speed);
        this->velocity = std::min(this->velocity, stop_limit);
    }

    void Train::update_rideable(
        TrackNetwork& network, Player& player, Interactables& interactables
    ) {
        const LocomotiveTypeInfo& loco_info = Train::locomotive_types()
            .at((size_t) this->loco_type);
        f64 yaw;
        Mat<4> inst = this->build_car_transform(
            network, 0, nullptr, nullptr, &yaw
        );
        this->rideable.position = (inst * loco_info.rideable.position.with(1.0))
            .swizzle<3>("xyz");
        this->rideable.angle = yaw + loco_info.rideable.angle;
        this->rideable.animation_id = loco_info.rideable.animation_id;
        if(this->interactable != nullptr) {
            this->interactable->pos = this->rideable.position;
        }
        bool show_interaction = player.riding != &this->rideable
            && this->interactable == nullptr;
        if(!show_interaction) { return; }
        this->interactable = interactables.create([this, p = &player]() {
            p->riding = &this->rideable;
            this->interactable = nullptr;
        });
    }

    static inline const f64 car_padding = 0.5;

    void Train::move_distance(
        const engine::Window& window, TrackNetwork& network, f64 distance
    ) {
        if(!this->current_path().has_value()) { return; }
        const AgentPath<TrackNetwork>& path = *this->current_path();
        bool at_end = false;
        TrackPosition front = this->cars.front().first
            .move_along(path, network, distance, &at_end);
        const Train::Car& first_car_info = this->car_at(0);
        f64 first_car_pading 
            = (first_car_info.length - first_car_info.axle_distance) / 2.0;
        f64 offset = first_car_pading;
        for(size_t car_i = 0; car_i < this->cars.size(); car_i += 1) {
            Train::CarPosition& car_pos = this->cars[car_i];
            const Train::Car& car_info = this->car_at(car_i);
            f64 padding = (car_info.length - car_info.axle_distance) / 2.0;
            car_pos.first = front
                .move_along(path, network, offset - padding);
            car_pos.second = car_pos.first
                .move_along(path, network, -car_info.axle_distance);
            offset -= car_info.length + car_padding;
        }
        if(at_end) {
            this->reached_target(window);
        }
    }

    static const f64 base_chugga_period = 0.5;
    static const f64 chugga_speed_factor = 1.0 / 5.0;

    void Train::update(
        TrackNetwork& network, engine::Scene& scene, 
        const engine::Window& window, ParticleManager* particles,
        Player& player, Interactables* interactables
    ) {
        this->release_unjustified_blocks(network);
        this->take_next_blocks(network);
        this->update_velocity(window, network);
        f64 moved_distance = this->velocity * window.delta_time();
        this->move_distance(window, network, moved_distance);
        if(interactables != nullptr) {
            this->update_rideable(network, player, *interactables);
        }
        this->speaker.position = this->current_position(network);
        this->speaker.update();
        bool play_whistle = this->current_state() != this->prev_state
            && this->prev_state == AgentState::Travelling
            && this->current_state() == AgentState::Loading
            && this->schedule.size() > 0;
        if(play_whistle) {
            this->speaker.pitch = Train::locomotive_types()
                .at((size_t) this->loco_type).whistle_pitch;
            this->speaker.play(scene.get(sound::train_whistle));
        }
        this->prev_state = this->current_state();
        f64 chugga_speed = this->velocity * chugga_speed_factor;
        f64 next_chugga_time = this->last_chugga_time 
            + base_chugga_period / chugga_speed;
        bool play_chugga = this->current_state() == AgentState::Travelling
            && next_chugga_time <= window.time()
            && this->velocity >= 1.0;
        if(play_chugga) {
            this->speaker.pitch = chugga_speed;
            this->speaker.play(scene.get(sound::chugga));
            this->last_chugga_time = window.time();
        }
        bool emit_smoke = play_chugga && particles != nullptr;
        if(emit_smoke) {
            const LocomotiveTypeInfo& loco_info = Train::locomotive_types()
                .at((size_t) this->loco_type);
            Mat<4> transform = this->build_car_transform(network, 0);
            Vec<4> smoke_pos = transform * loco_info.smoke_origin.with(1.0);
            particles->add(particle::random_smoke(network.rng)
                ->at(smoke_pos.swizzle<3>("xyz"))
            );
        }
    }

    static Character driver = Character(
        &human::female, &human::peasant_woman, 
        { 0, 0, 0 }, (u64) human::Animation::Sit
    );

    void Train::render(
        Renderer& renderer, TrackNetwork& network,
        engine::Scene& scene, const engine::Window& window
    ) {
        (void) network;
        (void) window;
        const LocomotiveTypeInfo& loco_info = Train::locomotive_types()
            .at((size_t) this->loco_type);
        for(size_t car_idx = 0; car_idx < this->cars.size(); car_idx += 1) {
            const Train::Car& car_info = this->car_at(car_idx);
            engine::Model& model = scene.get(car_info.model);
            Mat<4> transform = this->build_car_transform(network, car_idx);
            const engine::Animation& animation = model.animation("roll");
            f64 rolled_rotations = (window.time() * this->velocity)
                / (car_info.wheel_radius * 2 * pi);
            f64 timestamp = fmod(rolled_rotations, 1.0) * animation.length();
            renderer.render(
                model, std::array { transform }, &animation, timestamp
            );
        }
        driver.update(scene, window);
        for(const auto& driver_inst: loco_info.drivers) {
            f64 yaw;
            Mat<4> driver_transform 
                = this->build_car_transform(network, 0, nullptr, nullptr, &yaw);
            driver.position = (driver_transform * driver_inst.offset.with(1))
                .swizzle<3>("xyz");
            driver.angle = yaw + driver_inst.angle;
            driver.action 
                = Character::Action(driver_inst.animation_id, INFINITY);
            driver.render(scene, window, renderer);
        }
    }

}