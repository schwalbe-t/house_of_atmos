
#pragma once

#include "agent.hpp"
#include "terrain.hpp"

namespace houseofatmos::world {

    struct TileNetworkNode {
        using NodeId = std::pair<u64, u64>;
        struct NodeIdHash {
            std::size_t operator()(const NodeId& p) const {
                auto xh = std::hash<u64>{}(p.first);
                auto zh = std::hash<u64>{}(p.second);
                return xh ^ (zh + 0x9e3779b9 + (xh << 6) + (xh >> 2));
            }
        };
    };

    struct TileNetwork: AgentNetwork<TileNetworkNode> {

        u64 max_target_dist;

        TileNetwork(
            const Terrain* terrain, ComplexBank* complexes, 
            std::string local_lost_msg, u64 max_target_dist
        ): AgentNetwork(complexes, terrain, local_lost_msg), 
            max_target_dist(max_target_dist) {}

        TileNetwork(TileNetwork&& other) noexcept = default;
        TileNetwork& operator=(TileNetwork&& other) noexcept = default;

        virtual bool is_passable(NodeId node) = 0;


        static inline const u64 cost_ortho = 10;
        static inline const u64 cost_daigo = 14;

        void collect_next_nodes(
            std::optional<NodeId> prev, NodeId node, 
            std::vector<std::pair<NodeId, u64>>& out
        ) override {
            (void) prev;
            auto [x, z] = node;
            u64 left = x > 0? x - 1 : 0;
            u64 right = std::min(x + 1, this->terrain->width_in_tiles() - 1);
            u64 top = z > 0? z - 1 : 0;
            u64 bottom = std::min(z + 1, this->terrain->height_in_tiles() - 1);
            for(u64 nx = left; nx <= right; nx += 1) {
                for(u64 nz = top; nz <= bottom; nz += 1) {
                    NodeId neighbor = { nx, nz };
                    if(nx == x && nz == z) { continue; }
                    if(!this->is_passable(neighbor)) { continue; }
                    bool is_diagonal = nx != x && nz != z;
                    u64 cost = is_diagonal? cost_daigo : cost_ortho;
                    out.push_back({ neighbor, cost });
                }
            }
        }

        u64 node_target_dist(NodeId node, ComplexId target) override {
            auto [nx, nz] = node;
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

        bool node_at_target(NodeId node, ComplexId target) override {
            return this->node_target_dist(node, target) 
                <= this->max_target_dist;
        }

        NodeId tile_of(const Vec<3>& position) const {
            Vec<3> tile = position / this->terrain->units_per_tile();
            u64 x = (u64) std::max(tile.x(), 0.0);
            x = std::min(x, this->terrain->width_in_tiles());
            u64 z = (u64) std::max(tile.z(), 0.0);
            z = std::min(z, this->terrain->height_in_tiles());
            return NodeId(x, z);
        }

    };



    struct SerializedTileAgent {
        SerializedAgent agent;
        Vec<3> position;
        Vec<3> heading;
    };

    template<typename Network>
    struct TileAgent: Agent<Network> {

        Vec<3> position;

        private:
        size_t next_point_i = 0;
        Vec<3> heading = Vec<3>(0, 0, 1);

        public:
        TileAgent(Vec<3> position): position(position) {}
        TileAgent(
            const SerializedTileAgent& serialized, const engine::Arena& buffer
        ): Agent<Network>(serialized.agent, buffer) {
            this->position = serialized.position;
            this->heading = serialized.heading;
        }

        TileAgent(TileAgent&& other) noexcept = default;
        TileAgent& operator=(TileAgent&& other) noexcept = default;

        SerializedTileAgent serialize(engine::Arena& buffer) const {
            return {
                Agent<Network>::serialize(buffer), // this is a non-static
                this->position,
                this->heading
            };
        }

        const Vec<3>& current_heading() const { return this->heading; }

        Vec<3> current_position(Network& network) override {
            (void) network;
            return this->position;
        }

        std::optional<AgentPath<Network>> find_path_to(
            Network& network, ComplexId target
        ) override {
            this->next_point_i = 0;
            auto start = network.tile_of(this->position);
            return AgentPath<Network>::find(network, start, target);
        }

        void move_distance(
            const engine::Window& window, Network& network, f64 distance 
        ) {
            if(!this->current_path().has_value()) { return; }
            const AgentPath<Network>& path = *this->current_path();
            f64 remaining = distance;
            Vec<3> last_heading_point = this->position - this->heading;
            Vec<3> true_heading = this->heading;
            for(;;) {
                if(this->next_point_i >= path.points.size()) {
                    this->reached_target(window);
                    break;
                }
                auto [tgtt_x, tgtt_z] = path.points[this->next_point_i];
                Vec<3> target = Vec<3>(tgtt_x + 0.5, 0, tgtt_z + 0.5)
                    * network.terrain->units_per_tile();
                target.y() = network.terrain->elevation_at(target);
                Vec<3> step = target - this->position;
                f64 step_len = step.len();
                if(step_len != 0.0) { true_heading = step / step_len; }
                if(remaining >= step_len) {
                    remaining -= step_len;
                    this->next_point_i += 1;
                    this->position = target;
                    continue;
                }
                f64 step_progr = distance / step_len;
                this->position += step * step_progr;
                break;
            }
            this->heading = (this->position - last_heading_point).normalized();
            f64 heading_inacc = (this->heading - true_heading).abs().sum();
            if(heading_inacc > 1.5) { // should they differ too much, reset
                this->heading = true_heading; 
            }
        }
        
    };

}