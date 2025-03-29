
#pragma once

#include "agent.hpp"
#include "tile_network.hpp"

namespace houseofatmos::world {

    struct BoatNetwork: TileNetwork {

        BoatNetwork(const Terrain* terrain, ComplexBank* complexes):
            TileNetwork(terrain, complexes, "toast_boat_lost", 4) {}

        BoatNetwork(BoatNetwork&& other) noexcept = default;
        BoatNetwork& operator=(BoatNetwork&& other) noexcept = default;

        bool is_passable(NodeId node) override;

        void collect_node_points(
            std::optional<NodeId> prev, NodeId node, std::optional<NodeId> next,
            std::vector<Vec<3>>& out
        ) override;

    };



    struct Boat: Agent<BoatNetwork> {

        struct TypeInfo {
            std::string_view local_name;
            const ui::Background* icon;
            engine::Model::LoadArgs model;
            f64 weight_height_factor;
            u64 capacity;
            f64 speed;
            f64 cost;
        };

        static const std::vector<TypeInfo>& types();

        enum Type {
            Sail
        };


        static void load_resources(engine::Scene& scene) {
            for(const TypeInfo& type: Boat::types()) {
                scene.load(type.model);
            }
        }


        struct Serialized {
            SerializedAgent agent;
            Type type;
        };

        Type type;

        private:
        f64 yaw;

        public:
        Boat(Type type, Vec<3> position);
        Boat(
            const Serialized& serialized, const engine::Arena& buffer, 
            const Settings& settings
        );

        Boat(Boat&& other) noexcept = default;
        Boat& operator=(Boat&& other) noexcept = default;

        Serialized serialize(engine::Arena& buffer) const;



        f64 current_speed(BoatNetwork& network) override { 
            (void) network;
            return Boat::types().at((size_t) this->type).speed;
        }

        u64 item_storage_capacity() override {
            return Boat::types().at((size_t) this->type).capacity;
        }

        Vec<3> find_heading() const;

        void render(
            Renderer& renderer, BoatNetwork& network,
            engine::Scene& scene, const engine::Window& window
        ) override;

    };



    using BoatManager = AgentManager<Boat, BoatNetwork>;

}