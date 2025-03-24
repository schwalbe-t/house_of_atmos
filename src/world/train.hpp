
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

        std::optional<NodeId> closest_node_to(const Vec<3>& position) override;

    };



    struct Train: Agent<TrackNetwork> {

        struct Car {
            engine::Model::LoadArgs model;
            Vec<3> model_heading;
            f64 length;
            f64 front_axle, back_axle;
            f64 wheel_radius;
        };

        static inline const u64 train_car_cost = 250;

        struct LocomotiveTypeInfo {
            std::string_view local_name;
            const ui::Background* icon;
            std::vector<Car> loco_cars;
            const Car& car_type;
            u64 max_car_count;
            f64 speed;
            u64 cost;
        };

        static const std::vector<LocomotiveTypeInfo>& locomotive_types();

        enum LocomotiveType {
            Basic
        };


        static void load_resources(engine::Scene& scene) {
            for(const auto& loco_type: Train::locomotive_types()) {
                for(const auto& car: loco_type.loco_cars) {
                    scene.load(car.model);
                }
                scene.load(loco_type.car_type.model);
            }
        }


        struct Serialized {
            SerializedAgent agent;
            LocomotiveType locomotive;
            u64 car_count;
        };

        struct CarState {
            f64 yaw = 0.0, pitch = 0.0;
        };

        LocomotiveType loco_type;
        u64 car_count;
        std::vector<CarState> cars;
        
        Train(LocomotiveType loco_type, Vec<3> position);
        Train(const Serialized& serialized, const engine::Arena& buffer);

        Train(Train&& other) noexcept = default;
        Train& operator=(Train&& other) noexcept = default;

        Serialized serialize(engine::Arena& buffer) const;


        static inline const f64 car_padding = 0.75;

        f64 length() const { 
            const LocomotiveTypeInfo& loco_info = Train::locomotive_types()
                .at((size_t) this->loco_type);
            u64 car_count = loco_info.loco_cars.size() + this->car_count;
            return this->offset_of_car(car_count); 
        }

        f64 front_path_dist() const {
            return std::max(this->current_path_dist(), this->length());
        }

        const Car& car_at(size_t car_idx) const {
            const LocomotiveTypeInfo& loco_info = Train::locomotive_types()
                .at((size_t) this->loco_type);
            bool is_loco = car_idx < loco_info.loco_cars.size();
            if(is_loco) { return loco_info.loco_cars[car_idx]; }
            return loco_info.car_type;
        }

        f64 offset_of_car(size_t car_idx) const;


        f64 current_speed(TrackNetwork& network) override {
            (void) network;
            return Train::locomotive_types().at((size_t) this->loco_type).speed;
        }

        static inline const u64 item_capacity_per_car = 200;

        u64 item_storage_capacity() override {
            return this->car_count * Train::item_capacity_per_car;
        }

        void update(
            TrackNetwork& network, 
            engine::Scene& scene, const engine::Window& window
        ) override;

        Vec<3> find_heading(size_t car_idx) const;

        void render(
            Renderer& renderer, TrackNetwork& network,
            engine::Scene& scene, const engine::Window& window
        ) override;

    };


    using TrainManager = AgentManager<Train, TrackNetwork>;

}