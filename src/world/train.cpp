
#include "train.hpp"
#include "../particle_const.hpp"
#include <algorithm>

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

    void TrackNetwork::find_connections(NodeId node, Node& node_data) {
        u64 tiles_per_chunk = this->terrain->tiles_per_chunk();
        u64 world_wch = this->terrain->width_in_chunks();
        u64 world_hch = this->terrain->height_in_chunks();
        const std::vector<Vec<3>>& node_pts = TrackPiece::types()
            .at((size_t) node->type).points;
        Mat<4> node_inst = node->build_transform(
            node_data.chunk_x, node_data.chunk_z, tiles_per_chunk,
            this->terrain->units_per_tile()
        );
        u64 node_tx = node_data.chunk_x * tiles_per_chunk + node->x;
        u64 node_tz = node_data.chunk_z * tiles_per_chunk + node->z;
        u64 min_chx = (node_tx > 0? node_tx - 1 : 0) / tiles_per_chunk;
        u64 min_chz = (node_tz > 0? node_tz - 1 : 0) / tiles_per_chunk;
        u64 max_chx = std::min((node_tx + 1) / tiles_per_chunk, world_wch - 1);
        u64 max_chz = std::min((node_tz + 1) / tiles_per_chunk, world_hch - 1);
        for(u64 conn_chx = min_chx; conn_chx <= max_chx; conn_chx += 1) {
            for(u64 conn_chz = min_chz; conn_chz <= max_chz + 1; conn_chz += 1) {
                const Terrain::ChunkData& conn_chunk = this->terrain
                    ->chunk_at(conn_chx, conn_chz);
                for(const TrackPiece& conn_piece: conn_chunk.track_pieces) {
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
                        node_tx, node_tz, node->elevation, 
                        node_pts[0], node_inst
                    );
                    if(connected_low) { 
                        node_data.connected_low.push_back(&conn_piece); 
                    }
                    bool connected_high = track_piece_connected_to(
                        conn_pts, conn_inst, node_pts.back(), node_inst
                    ) && track_piece_connections_align(
                        *this->terrain, 
                        pc_tx, pc_tz, conn_piece.elevation, 
                        conn_pts, conn_inst,
                        node_tx, node_tz, node->elevation, 
                        node_pts.back(), node_inst
                    );
                    if(connected_high) {
                        node_data.connected_high.push_back(&conn_piece);
                    }
                }
            }
        }
    }

    static const u64 max_block_size = 4;

    void TrackNetwork::assign_to_blocks(NodeId piece, Block* previous) {
        Node& node = this->graph.at(piece);
        if(node.block != nullptr) { return; }
        bool is_conflict = piece->direction == TrackPiece::Any;
        bool add_to_prev = !is_conflict
            && previous != nullptr
            && previous->type != Block::Conflict
            && previous->size < max_block_size;
        node.block = previous;
        if(!add_to_prev) {
            Block::Type type = is_conflict? Block::Conflict : Block::Simple;
            this->blocks.push_back(Block(type));
            node.block = &this->blocks.back();
        }
        node.block->size += 1;
        for(NodeId connected: node.connected_low) {
            this->assign_to_blocks(connected, node.block);
        }
        for(NodeId connected: node.connected_high) {
            this->assign_to_blocks(connected, node.block);
        }
    }

    void TrackNetwork::reload() {
        this->graph.clear();
        u64 world_w_ch = this->terrain->width_in_chunks();
        u64 world_h_ch = this->terrain->height_in_chunks();
        for(u64 chunk_x = 0; chunk_x < world_w_ch; chunk_x += 1) {
            for(u64 chunk_z = 0; chunk_z < world_h_ch; chunk_z += 1) {
                const Terrain::ChunkData& chunk = this->terrain
                    ->chunk_at(chunk_x, chunk_z);
                for(const TrackPiece& track_piece: chunk.track_pieces) {
                    NodeId node_id = &track_piece;
                    auto node = Node(chunk_x, chunk_z, {}, {});
                    this->find_connections(node_id, node);
                    this->graph[node_id] = node;
                }
            }
        }
        this->blocks.clear();
        for(u64 chunk_x = 0; chunk_x < world_w_ch; chunk_x += 1) {
            for(u64 chunk_z = 0; chunk_z < world_h_ch; chunk_z += 1) {
                const Terrain::ChunkData& chunk = this->terrain
                    ->chunk_at(chunk_x, chunk_z);
                for(const TrackPiece& track_piece: chunk.track_pieces) {
                    this->assign_to_blocks(&track_piece);
                }
            }
        }
    }


    void TrackNetwork::collect_next_nodes(
        std::optional<NodeId> prev, NodeId node, 
        std::vector<std::pair<NodeId, u64>>& out
    ) {
        Node node_d = this->graph[node];
        const std::vector<NodeId>& low = node_d.connected_low;
        const std::vector<NodeId>& high = node_d.connected_high;
        // previous piece is connected at LOW end of this piece?
        // -> connects to all pieces at HIGH end
        bool prev_at_low = !prev.has_value()
            || std::find(low.begin(), low.end(), *prev) != low.end();
        bool is_ascending = node->direction == TrackPiece::Ascending
            || node->direction == TrackPiece::Any;
        if(prev_at_low && is_ascending) {
            for(NodeId c: high) { out.push_back({ c, 1 }); }
        }
        // previous piece is connected at HIGH end of this piece?
        // -> connects to all pieces at LOW end
        bool prev_at_high = !prev.has_value()
            || std::find(high.begin(), high.end(), *prev) != high.end();
        bool is_descending = node->direction == TrackPiece::Descending
            || node->direction == TrackPiece::Any;
        if(prev_at_high && is_descending) {
            for(NodeId c: low) { out.push_back({ c, 1 }); }
        }
    }

    u64 TrackNetwork::node_target_dist(NodeId node, ComplexId target) {
        // find node position
        Node node_data = this->graph[node];
        u64 nx = node_data.chunk_x * this->terrain->tiles_per_chunk() + node->x;
        u64 nz = node_data.chunk_z * this->terrain->tiles_per_chunk() + node->z;
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

    void TrackNetwork::collect_node_points(
        std::optional<NodeId> prev, NodeId node, std::optional<NodeId> next,
        std::vector<Vec<3>>& out
    ) {
        u64 t_p_ch = this->terrain->tiles_per_chunk();
        u64 u_p_t = this->terrain->units_per_tile();
        Node node_data = this->graph[node];
        Mat<4> node_inst = node->build_transform(
            node_data.chunk_x, node_data.chunk_z, t_p_ch, u_p_t
        );
        std::vector<Vec<3>> points = TrackPiece::types()
            .at((size_t) node->type).points;
        if(prev.has_value()) {
            NodeId prev_id = *prev;
            Node prev_data = this->graph[prev_id];
            std::vector<Vec<3>> prev_pts = TrackPiece::types()
                .at((size_t) prev_id->type).points;
            Mat<4> prev_inst = prev_id->build_transform(
                prev_data.chunk_x, prev_data.chunk_z, t_p_ch, u_p_t
            );
            bool connected = track_piece_connected_to(
                prev_pts, prev_inst, points[0], node_inst
            );
            if(!connected) { std::reverse(points.begin(), points.end()); }
        }
        if(next.has_value()) {
            NodeId next_id = *next;
            Node next_data = this->graph[next_id];
            std::vector<Vec<3>> next_pts = TrackPiece::types()
                .at((size_t) next_id->type).points;
            Mat<4> next_inst = next_id->build_transform(
                next_data.chunk_x, next_data.chunk_z, t_p_ch, u_p_t
            );
            bool connected = track_piece_connected_to(
                next_pts, next_inst, points.back(), node_inst
            );
            if(!connected) { std::reverse(points.begin(), points.end()); }
        }
        for(Vec<3> model_point: points) {
            Vec<4> world_point = node_inst * model_point.with(1.0);
            out.push_back(world_point.swizzle<3>("xyz"));
        }
    }

    std::optional<TrackNetwork::NodeId> TrackNetwork::closest_node_to(
        const Vec<3>& position
    ) {
        Vec<3> tile = position / this->terrain->units_per_tile();
        i64 cx = (i64) tile.x();
        i64 cz = (i64) tile.z();
        std::vector<TrackPiece*> pieces;
        for(i64 cd = 0; cd < 5; cd += 1) {
            for(i64 ox = -cd; ox <= cd; ox += 1) {
                for(i64 oz = -cd; oz <= cd; oz += 1) {
                    bool is_inside = std::abs(ox) != cd && std::abs(oz) != cd;
                    if(is_inside) { continue; }
                    pieces.clear();
                    this->terrain->track_pieces_at(cx + ox, cz + oz, &pieces);
                    if(pieces.size() == 0) { continue; }
                    return pieces[0];
                }
            }
        }
        return std::nullopt;
    }



    static Train::Car train_car_old = Train::Car(
        engine::Model::LoadArgs(
            "res/trains/train_car_old.glb", Renderer::model_attribs,
            engine::FaceCulling::Enabled
        ),
        Vec<3>(0, 0, 1), // model heading
        3.6, // length
        0.8, 2.8, // offsets of the front and back axles
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
        0.85, 3.25, // offsets of the front and back axles
        0.5, // wheel radius
        250 // item capacity
    );

    static Train::Car train_car_tram = Train::Car(
        engine::Model::LoadArgs(
            "res/trains/tram.glb", 
            Renderer::model_attribs,
            engine::FaceCulling::Disabled
        ),
        Vec<3>(0, 0, 1), // model heading
        5.0, // length
        1.1, 3.9, // offsets of the front and back axles
        0.5, // wheel radius
        0 // item capacity
    );

    static std::vector<Train::LocomotiveTypeInfo> locomotive_infos = {
        /* Basic */ {
            "locomotive_name_basic",
            &ui_icon::basic_locomotive,
            {
                Train::Car(
                    engine::Model::LoadArgs(
                        "res/trains/basic_locomotive.glb", 
                        Renderer::model_attribs,
                        engine::FaceCulling::Disabled
                    ),
                    Vec<3>(0, 0, 1), // model heading
                    3.62, // length
                    0.96, 2.17, // offsets of the front and back axles
                    0.9, // wheel radius (of big wheel)
                    50 // item capacity
                )
            },
            train_car_old,
            Vec<3>(0.0, 3.0, 1.22), // relative smoke origin
            1.4, // whistle pitch
            3, // max car count
            5.0, // speed
            5000 // cost
        },
        /* Small */ {
            "locomotive_name_small",
            &ui_icon::small_locomotive,
            {
                Train::Car(
                    engine::Model::LoadArgs(
                        "res/trains/small_locomotive.glb", 
                        Renderer::model_attribs,
                        engine::FaceCulling::Disabled
                    ),
                    Vec<3>(0, 0, 1), // model heading
                    5.0, // length
                    1.0, 3.4, // offsets of the front and back axles
                    0.9, // wheel radius (of big wheels)
                    50 // item capacity
                )
            },
            train_car_old,
            Vec<3>(0.0, 2.6, 2.0), // relative smoke origin
            1.3, // whistle pitch
            4, // max car count
            7.5, // speed
            7500 // cost
        },
        /* Tram */ {
            "locomotive_name_tram",
            &ui_icon::tram,
            { train_car_tram },
            train_car_tram,
            Vec<3>(0.395, 3.087, -1.702), // relative smoke origin
            1.35, // whistle pitch
            2, // max car count
            7.0, // speed
            6500 // cost
        }
    };

    const std::vector<Train::LocomotiveTypeInfo>& Train::locomotive_types() {
        return locomotive_infos;
    }


    Train::Train(
        LocomotiveType loco_type, Vec<3> position, const Settings& settings
    ) {
        this->position = position;
        this->loco_type = loco_type;
        this->car_count = 0;
        this->speaker.volume = settings.sfx_volume;
    }

    Train::Train(
        const Serialized& serialized, const engine::Arena& buffer,
        const Settings& settings
    ): 
        Agent<TrackNetwork>(serialized.agent, buffer) {
        this->loco_type = serialized.locomotive;
        this->car_count = serialized.car_count;
        const LocomotiveTypeInfo& loco_info = Train::locomotive_types()
            .at((size_t) this->loco_type);
        this->cars.resize(loco_info.loco_cars.size() + this->car_count);
        this->speaker.volume = settings.sfx_volume;
    }

    Train::Serialized Train::serialize(engine::Arena& buffer) const {
        return Serialized(
            Agent<TrackNetwork>::serialize(buffer), // this is a non-static
            this->loco_type,
            this->car_count
        );
    }

    f64 Train::offset_of_car(size_t car_idx) const {
        const LocomotiveTypeInfo& loco_info = Train::locomotive_types()
            .at((size_t) this->loco_type);
        size_t remaining_cars = car_idx;
        f64 length_sum = 0.0;
        for(const Car& car: loco_info.loco_cars) {
            if(remaining_cars == 0) { break; }
            length_sum += car.length;
            remaining_cars -= 1;
        }
        length_sum += remaining_cars * loco_info.car_type.length;
        f64 padding = car_idx * car_padding;
        return length_sum + padding;
    }

    void Train::manage_block_ownership(TrackNetwork& network) {
        f64 curr_front_dist = this->front_path_dist();
        f64 curr_back_dist = std::max(
            curr_front_dist - this->length() - 5.0, 0.0
        );
        const auto& sections = this->current_path().sections;
        size_t back_sect = this->current_path().after(curr_back_dist).second;
        size_t front_sect = this->current_path().after(curr_front_dist).second;
        size_t next_sect = front_sect + 1;
        for(OwnedBlock& owning: this->owning_blocks) {
            owning.justified = false;
        }
        bool may_take_all = true;
        for(size_t sect_i = back_sect; sect_i < next_sect; sect_i += 1) {
            const auto& section = sections[sect_i];
            const TrackNetwork::Node& node = network.graph.at(section.node);
            if(node.block->owner != this) { continue; }
            auto owned_block = std::find_if(
                this->owning_blocks.begin(), this->owning_blocks.end(), 
                [n = &node](const auto& b) { return b.block == n->block; }
            );
            if(owned_block == this->owning_blocks.end()) { continue; }
            owned_block->justified = true;
        }
        for(size_t sect_i = next_sect; sect_i < sections.size(); sect_i += 1) {
            const auto& section = sections[sect_i];
            const TrackNetwork::Node& node = network.graph.at(section.node);
            if(node.block->owner == this) { 
                auto owned_block = std::find_if(
                    this->owning_blocks.begin(), this->owning_blocks.end(), 
                    [n = &node](const auto& b) { return b.block == n->block; }
                );
                owned_block->justified = true;
                continue;
            }
            if(!node.block->in_queue(this)) { node.block->await(this); }
            may_take_all &= node.block->may_take(this);
            if(node.block->type == TrackNetwork::Block::Conflict) { continue; }
            break;
        }
        if(!may_take_all) { return; }
        for(size_t sect_i = next_sect; sect_i < sections.size(); sect_i += 1) {
            const auto& section = sections[sect_i];
            const TrackNetwork::Node& node = network.graph.at(section.node);
            if(!node.block->may_take(this)) { break; }
            node.block->take(this);
            this->owning_blocks.push_back(OwnedBlock(node.block, true));
        }
        for(OwnedBlock& owning: this->owning_blocks) {
            if(owning.justified) { continue; }
            owning.block->release(this);
        }
        auto new_owned_end = std::remove_if(
            this->owning_blocks.begin(), this->owning_blocks.end(),
            [](const auto& b) { return !b.justified; }
        );
        this->owning_blocks.erase(new_owned_end, this->owning_blocks.end());
    }

    static const f64 base_chugga_period = 0.5;
    static const f64 chugga_speed_factor = 1.0 / 5.0;

    void Train::update(
        TrackNetwork& network, engine::Scene& scene, 
        const engine::Window& window, ParticleManager* particles
    ) {
        this->manage_block_ownership(network);
        this->speaker.position = this->position;
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
        f64 chugga_speed = this->current_speed(network) * chugga_speed_factor;
        f64 next_chugga_time = this->last_chugga_time 
            + base_chugga_period / chugga_speed;
        bool play_chugga = this->current_state() == AgentState::Travelling
            && next_chugga_time <= window.time();
        if(play_chugga) {
            this->speaker.pitch = chugga_speed;
            this->speaker.play(scene.get(sound::chugga));
            this->last_chugga_time = window.time();
        }
        bool emit_smoke = play_chugga && this->cars.size() > 0
            && particles != nullptr;
        if(emit_smoke) {
            const LocomotiveTypeInfo& loco_info = Train::locomotive_types()
                .at((size_t) this->loco_type);
            const Train::Car& car_info = this->car_at(0);
            f64 car_center = this->front_path_dist()
                - this->offset_of_car(0) 
                - (car_info.length / 2.0);
            Vec<3> position = this->current_path().after(car_center).first;
            Mat<4> transform
                = Mat<4>::translate(position)
                * Mat<4>::rotate_y(this->cars[0].yaw) 
                * Mat<4>::rotate_x(this->cars[0].pitch);
            Vec<4> smoke_pos = transform * loco_info.smoke_origin.with(1.0);
            particles->add(particle::random_smoke(network.rng)
                ->at(smoke_pos.swizzle<3>("xyz"))
            );
        }
    }

    Vec<3> Train::find_heading(size_t car_idx) const {
        const LocomotiveTypeInfo& loco_info = Train::locomotive_types()
            .at((size_t) this->loco_type);
        f64 train_front = this->front_path_dist();
        f64 car_start = train_front - this->offset_of_car(car_idx);
        Vec<3> front = this->current_path()
            .after(car_start - loco_info.car_type.front_axle).first;
        Vec<3> back = this->current_path()
            .after(car_start - loco_info.car_type.back_axle).first;
        return (front - back).normalized();
    }

    void Train::render(
        Renderer& renderer, TrackNetwork& network,
        engine::Scene& scene, const engine::Window& window
    ) {
        (void) network;
        (void) window;
        const LocomotiveTypeInfo& loco_info = Train::locomotive_types()
            .at((size_t) this->loco_type);
        this->cars.resize(loco_info.loco_cars.size() + this->car_count);
        f64 train_front = this->front_path_dist();
        for(size_t car_idx = 0; car_idx < this->cars.size(); car_idx += 1) {
            const Train::Car& car_info = this->car_at(car_idx);
            f64 car_center = train_front
                - this->offset_of_car(car_idx) 
                - (car_info.length / 2.0);
            CarState& car = this->cars[car_idx];
            if(this->current_state() == AgentState::Travelling) {
                Vec<3> heading = this->find_heading(car_idx);
                auto [pitch, yaw] = Agent<TrackNetwork>
                    ::compute_heading_angles(heading, car_info.model_heading);
                car.pitch = pitch;
                car.yaw = yaw;
            }
            engine::Model& model = scene.get(car_info.model);
            Vec<3> position = this->current_path().after(car_center).first;
            Mat<4> transform
                = Mat<4>::translate(position)
                * Mat<4>::rotate_y(car.yaw) 
                * Mat<4>::rotate_x(car.pitch);
            const engine::Animation& animation = model.animation("roll");
            f64 rolled_rotations = train_front 
                / (car_info.wheel_radius * 2 * pi);
            f64 timestamp = fmod(rolled_rotations, 1.0) * animation.length();
            renderer.render(
                model, std::array { transform }, &animation, timestamp
            );
        }
    }

}