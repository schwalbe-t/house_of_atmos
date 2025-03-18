
#pragma once

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

        const Terrain* terrain;
        StatefulRNG rng;

        CarriageNetwork(const Terrain* terrain, ComplexBank* complexes): 
            AgentNetwork(complexes, "toast_carriage_lost"), terrain(terrain) {}

        CarriageNetwork(CarriageNetwork&& other) noexcept = default;
        CarriageNetwork& operator=(CarriageNetwork&& other) noexcept = default;

        bool is_passable(NodeId node) const;

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



    struct Carriage: Agent<CarriageNetwork> {
        
        static const inline engine::Model::LoadArgs horse_model = {
            "res/entities/horse.glb", Renderer::model_attribs,
            engine::FaceCulling::Disabled
        };

        struct HorseTypeInfo {
            engine::Texture::LoadArgs texture;
        };

        static const std::vector<HorseTypeInfo>& horse_types();

        enum struct HorseType {
            White,
            WhiteSpotted,
            Brown,
            BrownSpotted,
            BlackSpotted
        };


        struct CarriageTypeInfo {
            struct Driver {
                Vec<3> offset;
                f64 angle;
            };
            
            std::string_view local_name;
            const ui::Background* icon;
            engine::Model::LoadArgs model;
            Vec<3> carriage_offset;
            std::vector<Vec<3>> horse_offsets;
            std::vector<Driver> drivers;
            f64 wheel_radius;
            u64 capacity;
            f64 speed;
            u64 cost;
        };

        static const std::vector<CarriageTypeInfo>& carriage_types();

        enum CarriageType {
            Round
        };


        static void load_resources(engine::Scene& scene) {
            scene.load(Carriage::horse_model);
            for(const HorseTypeInfo& horse_type: Carriage::horse_types()) {
                scene.load(horse_type.texture);
            }
            for(const CarriageTypeInfo& carriage_type: Carriage::carriage_types()) {
                scene.load(carriage_type.model);
            }
        }


        struct Serialized {
            SerializedAgent agent;
            CarriageType type;
            u64 horses_count, horses_offset;
        };

        CarriageType type;
        std::vector<HorseType> horses;

        private:
        engine::Speaker speaker = engine::Speaker(
            engine::Speaker::Space::World, 5.0
        );
        f64 yaw = 0.0, pitch = 0.0;
        AgentState prev_state = AgentState::Travelling;
        f64 last_step_time = 0.0;

        public:
        Carriage(CarriageType type, Vec<3> position, StatefulRNG& rng);
        Carriage(const Serialized& serialized, const engine::Arena& buffer);

        Carriage(Carriage&& other) noexcept = default;
        Carriage& operator=(Carriage&& other) noexcept = default;

        Serialized serialize(engine::Arena& buffer) const;


        f64 current_speed(CarriageNetwork& network) override { 
            (void) network;
            return Carriage::carriage_types().at((size_t) this->type).speed;
        }

        u64 item_storage_capacity() override {
            return Carriage::carriage_types().at((size_t) this->type).capacity;
        }

        void update(
            CarriageNetwork& network, 
            engine::Scene& scene, const engine::Window& window
        ) override;

        Vec<3> find_heading() const;

        private:
        void render_horses(
            Renderer& renderer, engine::Scene& scene, 
            const engine::Window& window
        ) const;
        void render_drivers(
            Renderer& renderer, engine::Scene& scene, 
            const engine::Window& window
        ) const;
        void render_carriage(Renderer& renderer, engine::Scene& scene) const;

        public:
        void render(
            Renderer& renderer, CarriageNetwork& network,
            engine::Scene& scene, const engine::Window& window
        ) override;

    };



    using CarriageManager = AgentManager<Carriage, CarriageNetwork>;

}