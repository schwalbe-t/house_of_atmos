
#include "train.hpp"
#include <algorithm>

namespace houseofatmos::world {

    static const f64 max_piece_point_dist = 0.001;

    static bool track_pieces_connected(
        std::span<const Vec<3>> points_a, const Mat<4>& inst_a,
        std::span<const Vec<3>> points_b, const Mat<4>& inst_b 
    ) {
        Vec<3> a_end_f = (inst_a * points_a[0].with(1.0)).swizzle<3>("xyz");
        Vec<3> a_end_b = (inst_a * points_a.back().with(1.0)).swizzle<3>("xyz");
        Vec<3> b_end_f = (inst_b * points_b[0].with(1.0)).swizzle<3>("xyz");
        Vec<3> b_end_b = (inst_b * points_b.back().with(1.0)).swizzle<3>("xyz");
        return (a_end_f - b_end_f).len() <= max_piece_point_dist
            || (a_end_f - b_end_b).len() <= max_piece_point_dist
            || (a_end_b - b_end_f).len() <= max_piece_point_dist
            || (a_end_b - b_end_b).len() <= max_piece_point_dist;
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
                    if(&conn_piece == node) { continue; }
                    const std::vector<Vec<3>>& conn_pts = TrackPiece::types()
                        .at((size_t) conn_piece.type).points;
                    Mat<4> conn_inst = conn_piece.build_transform(
                        conn_chx, conn_chz, tiles_per_chunk, 
                        this->terrain->units_per_tile()
                    );
                    bool is_connected = track_pieces_connected(
                        node_pts, node_inst, conn_pts, conn_inst
                    );
                    if(!is_connected) { continue; }
                    node_data.connect_to.push_back(&conn_piece);
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
                    auto node = Node(chunk_x, chunk_z, {});
                    this->find_connections(node_id, node);
                    this->graph[node_id] = node;
                }
            }
        }
    }


    void TrackNetwork::collect_next_nodes(
        NodeId node, std::vector<std::pair<NodeId, u64>>& out
    ) {
        if(node == nullptr) { return; }
        Node node_data = this->graph[node];
        for(NodeId connected: node_data.connect_to) {
            out.push_back({ connected, 1 });
        }
    }

    u64 TrackNetwork::node_target_dist(NodeId node, ComplexId target) {
        if(node == nullptr) { return UINT64_MAX; }
        Node node_data = this->graph[node];
        u64 nx = node_data.chunk_x * this->terrain->tiles_per_chunk() + node->x;
        u64 nz = node_data.chunk_z * this->terrain->tiles_per_chunk() + node->z;
        const Complex& complex = this->complexes->get(target);
        auto [cx, cz] = complex.closest_member_to(nx, nz);
        u64 dx = (u64) std::abs((i64) nx - (i64) cx);
        u64 dz = (u64) std::abs((i64) nz - (i64) cz);
        return dx + dz;
    }

    bool TrackNetwork::node_at_target(NodeId node, ComplexId target) {
        return this->node_target_dist(node, target) <= 3;
    }

    void TrackNetwork::collect_node_points(
        std::optional<NodeId> prev, NodeId node, std::optional<NodeId> next,
        std::vector<Vec<3>>& out
    ) {
        if(node == nullptr) { return; }
        u64 t_p_ch = this->terrain->tiles_per_chunk();
        u64 u_p_t = this->terrain->units_per_tile();
        Node node_data = this->graph[node];
        Mat<4> node_inst = node->build_transform(
            node_data.chunk_x, node_data.chunk_z, t_p_ch, u_p_t
        );
        std::vector<Vec<3>> points = TrackPiece::types()
            .at((size_t) node->type).points;
        if(prev.has_value()) {
            NodeId prev_id = *next;
            Node prev_data = this->graph[prev_id];
            std::vector<Vec<3>> prev_pts = TrackPiece::types()
                .at((size_t) prev_id->type).points;
            Mat<4> prev_inst = prev_id->build_transform(
                prev_data.chunk_x, prev_data.chunk_z, t_p_ch, u_p_t
            );
            const Vec<3>* first_node_pt = &points[0];
            bool connected = track_pieces_connected(
                prev_pts, prev_inst, { first_node_pt, 1 }, node_inst
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
            const Vec<3>* last_node_pt = &points.back();
            bool connected = track_pieces_connected(
                next_pts, next_inst, { last_node_pt, 1 }, node_inst
            );
            if(!connected) { std::reverse(points.begin(), points.end()); }
        }
        for(Vec<3> model_point: points) {
            Vec<4> world_point = node_inst * model_point.with(1.0);
            out.push_back(world_point.swizzle<3>("xyz"));
        }
    }

    TrackNetwork::NodeId TrackNetwork::closest_node_to(const Vec<3>& position) {
        Vec<3> tile = position / this->terrain->units_per_tile();
        std::vector<const TrackPiece*> pieces;
        this->terrain->track_pieces_at((i64) tile.x(), (i64) tile.z(), &pieces);
        if(pieces.size() == 0) { return nullptr; }
        return pieces[0];
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
        f64 train_front = std::max(this->current_path_dist(), this->length());
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
        f64 train_front = std::max(this->current_path_dist(), this->length());
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