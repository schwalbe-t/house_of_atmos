
#pragma once

#include "agents.hpp"
#include "terrain.hpp"

namespace houseofatmos::world {

    struct TrackNetworkNode {
        using NodeId = const TrackPiece*;
        struct NodeIdHash {
            std::size_t operator()(const NodeId& p) const { 
                return (size_t) p; 
            }
        };
    };

    struct TrackNetwork: AgentNetwork<TrackNetworkNode> {
        
        struct Node {
            u64 chunk_x, chunk_z;
            std::vector<NodeId> connect_to;
        };
        
        const Terrain* terrain;
        std::unordered_map<NodeId, Node, NodeIdHash> graph;

        TrackNetwork(const Terrain* terrain, ComplexBank* complexes):
            AgentNetwork(complexes, "toast_train_lost"), terrain(terrain) {}

        TrackNetwork(TrackNetwork&& other) noexcept = default;
        TrackNetwork& operator=(TrackNetwork&& other) noexcept = default;


        private:
        void find_connections(NodeId node, Node& node_data);
        public:
        void reload() override;


        void collect_next_nodes(
            NodeId node, std::vector<std::pair<NodeId, u64>>& out
        ) override;

        u64 node_target_dist(NodeId node, ComplexId target) override;

        bool node_at_target(NodeId node, ComplexId target) override;

        void collect_node_points(
            std::optional<NodeId> prev, NodeId node, std::optional<NodeId> next,
            std::vector<Vec<3>>& out
        ) override;

        NodeId closest_node_to(const Vec<3>& position) override;

    };



    struct Train: Agent<TrackNetwork> {

        struct Serialized {
            SerializedAgent agent;
        };
        
        Train(Vec<3> position);
        Train(const Serialized& serialized, const engine::Arena& buffer);

        Train(Train&& other) noexcept = default;
        Train& operator=(Train&& other) noexcept = default;

        Serialized serialize(engine::Arena& buffer) const;


        f64 current_speed(TrackNetwork& network) override {
            (void) network;
            return 7.5;
        }

        u64 item_storage_capacity() override {
            return 500;
        }

        void update(
            TrackNetwork& network, 
            engine::Scene& scene, const engine::Window& window
        ) override;

        Vec<3> find_heading() const;

        void render(
            Renderer& renderer, TrackNetwork& network,
            engine::Scene& scene, const engine::Window& window
        ) override;

    };


    using TrainManager = AgentManager<Train, TrackNetwork>;

}