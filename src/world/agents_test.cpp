
#include "agents.hpp"
#include "terrain.hpp"

namespace houseofatmos::world {

    struct CarriageNetworkNode {
        using NodeId = std::pair<u64, u64>;
        struct NodeIdHash {
            std::size_t operator()(const NodeId& p) const {
                auto xh = std::hash<u64>{}(p.first);
                auto zh = std::hash<u64>{}(p.second);
                return xh ^ (zh + 0x9e3779b9 + (xh << 6) + (xh >> 2));
            }
        };
    };

    struct CarriageNetwork: AgentNetwork<CarriageNetworkNode> {
        
        const Terrain& terrain;
        StatefulRNG rng;

        CarriageNetwork(const Terrain& terrain, const ComplexBank& complexes): 
            AgentNetwork(complexes), terrain(terrain) {}

        private:
        std::vector<std::pair<NodeId, u64>> target_search_tiles;
        
        bool is_passable(NodeId node) const {
            auto [x, z] = node;
            return this->terrain.path_at((i64) x, (i64) z)
                && this->terrain.building_at((i64) x, (i64) z) == nullptr;
        }
        
        public:
        static inline const u64 cost_ortho = 10;
        static inline const u64 cost_daigo = 14;

        void collect_next_nodes(
            NodeId node, std::vector<std::pair<NodeId, u64>>& out
        ) override {
            auto [x, z] = node;
            u64 left = x > 0? x - 1 : 0;
            u64 right = std::min(x + 1, this->terrain.width_in_tiles() - 1);
            u64 top = z > 0? z - 1 : 0;
            u64 bottom = std::min(z + 1, this->terrain.height_in_tiles() - 1);
            for(u64 nx = left; nx <= right; nx += 1) {
                for(u64 nz = top; nz <= bottom; nz += 1) {
                    NodeId neighbor = { nx, nz };
                    if(nx == x && nz == z) { continue; }
                    if(!this->is_passable(neighbor)) { continue; }
                    bool is_diagonal = nx != x && nz != z;
                    u64 cost = nx == is_diagonal? cost_daigo : cost_ortho;
                    out.push_back({ neighbor, cost });
                }
            }
        }

        u64 node_target_dist(NodeId node, ComplexId target) override {
            auto [nx, nz] = node;
            const Complex& complex = this->complexes.get(target);
            auto [cx, cz] = complex.closest_member_to(node.first, node.second);
            u64 dx = (u64) std::abs((i64) nx - (i64) cx);
            u64 dz = (u64) std::abs((i64) nz - (i64) cz);
            u64 diagonal = std::min(dx, dz);
            return (dx - diagonal) * cost_ortho + (dz - diagonal) * cost_ortho 
                + diagonal * cost_daigo;
        }

        bool node_at_target(NodeId node, ComplexId target) override {
            this->target_search_tiles.clear();
            this->collect_next_nodes(node, this->target_search_tiles);
            for(auto connected: this->target_search_tiles) {
                auto [x, z] = connected.first;
                const Building* building = this->terrain
                    .building_at((i64) x, (i64) z);
                bool is_valid = building != nullptr
                    && building->complex.has_value()
                    && building->complex->index == target.index;
                if(is_valid) { return true; }
            }
            return false;
        }

        static inline const f64 max_path_var = 0.05;

        void collect_node_points(
            std::optional<NodeId> prev, NodeId node, std::optional<NodeId> next,
            std::vector<Vec<3>>& out
        ) override {
            (void) prev;
            (void) next;
            auto [x, z] = node;
            Vec<3> o = Vec<3>(this->rng.next_f64(), 0, this->rng.next_f64());
            o = (o * 2.0 - Vec<3>(1.0, 0.0, 1.0)) * max_path_var;
            Vec<3> point = Vec<3>(x, 0, z) * this->terrain.units_per_tile()
                + (o + Vec<3>(0.5, 0, 0.5)) * this->terrain.units_per_tile();
            point.y() = this->terrain.elevation_at(point);
            out.push_back(point);
        }

        NodeId closest_node_to(const Vec<3>& position) override {
            Vec<3> tile = position / this->terrain.units_per_tile();
            u64 x = (u64) std::max(tile.x(), 0.0);
            x = std::min(x, this->terrain.width_in_tiles());
            u64 z = (u64) std::max(tile.z(), 0.0);
            z = std::min(z, this->terrain.height_in_tiles());
            return { x, z };
        }

    };

    struct Carriage: Agent<CarriageNetwork> {
        
        f64 current_speed(CarriageNetwork& network) override { 
            (void) network;
            return 4.0; 
        }

        u64 item_storage_capacity() override {
            return 100;
            // TODO! actual count depending on type of carriage
        }

        void render(
            Renderer& renderer, engine::Scene& scene, 
            const engine::Window& window
        ) override {
            (void) renderer;
            (void) window;
            (void) scene;
            // TODO! update sound
            // TODO! render carriage depending on type of carriage
        }

    };

    using CarriageManager = AgentManager<CarriageNetwork, Carriage>;

}