
#include "train.hpp"
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
                    bool same_chunk = conn_chx == node_data.chunk_x
                        && conn_chz == node_data.chunk_z;
                    bool same_tile = conn_piece.x == node->x
                        && conn_piece.z == node->z;
                    if(same_chunk && same_tile) { continue; }
                    const std::vector<Vec<3>>& conn_pts = TrackPiece::types()
                        .at((size_t) conn_piece.type).points;
                    Mat<4> conn_inst = conn_piece.build_transform(
                        conn_chx, conn_chz, tiles_per_chunk, 
                        this->terrain->units_per_tile()
                    );
                    bool connected_low = track_piece_connected_to(
                        conn_pts, conn_inst, node_pts[0], node_inst
                    );
                    if(connected_low) { 
                        node_data.connected_low.push_back(&conn_piece); 
                    }
                    bool connected_high = track_piece_connected_to(
                        conn_pts, conn_inst, node_pts.back(), node_inst
                    );
                    if(connected_high) {
                        node_data.connected_high.push_back(&conn_piece);
                    }
                }
            }
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
        if(prev_at_low) {
            for(NodeId c: high) { out.push_back({ c, 1 }); }
        }
        // previous piece is connected at HIGH end of this piece?
        // -> connects to all pieces at LOW end
        bool prev_at_high = !prev.has_value()
            || std::find(high.begin(), high.end(), *prev) != high.end();
        if(prev_at_high) {
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
        std::vector<const TrackPiece*> pieces;
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



    static Train::Car train_car_modern = Train::Car(
        engine::Model::LoadArgs(
            "res/trains/train_car_modern.glb", Renderer::model_attribs,
            engine::FaceCulling::Enabled
        ),
        Vec<3>(0, 0, 1), // model heading
        4.0, // length
        0.85, 3.25, // offsets of the front and back axles
        0.5 // wheel radius
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
                    5.0, // length
                    1.85, 3.05, // offsets of the front and back axles
                    0.9 // wheel radius (of big wheel)
                )
            },
            train_car_modern,
            3, // max car count
            5.0, // speed
            5000 // cost
        }
    };

    const std::vector<Train::LocomotiveTypeInfo>& Train::locomotive_types() {
        return locomotive_infos;
    }


    Train::Train(LocomotiveType loco_type, Vec<3> position) {
        this->position = position;
        this->loco_type = loco_type;
        this->car_count = 0;
    }

    Train::Train(const Serialized& serialized, const engine::Arena& buffer): 
        Agent<TrackNetwork>(serialized.agent, buffer) {
        this->loco_type = serialized.locomotive;
        this->car_count = serialized.car_count;
        const LocomotiveTypeInfo& loco_info = Train::locomotive_types()
            .at((size_t) this->loco_type);
        this->cars.resize(loco_info.loco_cars.size() + this->car_count);
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
        f64 padding = (std::max(car_idx, (u64) 1) - 1) * car_padding;
        return length_sum + padding;
    }

    void Train::update(
        TrackNetwork& network, 
        engine::Scene& scene, const engine::Window& window
    ) {
        (void) network;
        (void) scene;
        (void) window;
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