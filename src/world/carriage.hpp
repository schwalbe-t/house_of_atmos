
#pragma once

#include "agent.hpp"
#include "tile_network.hpp"

namespace houseofatmos::world {

    struct CarriageNetwork: TileNetwork {

        StatefulRNG rng;

        CarriageNetwork(const Terrain* terrain, ComplexBank* complexes): 
            TileNetwork(terrain, complexes, "toast_carriage_lost", 1) {}

        CarriageNetwork(CarriageNetwork&& other) noexcept = default;
        CarriageNetwork& operator=(CarriageNetwork&& other) noexcept = default;

        bool is_passable(NodeId node) override;

        void collect_node_points(
            std::optional<NodeId> prev, NodeId node, std::optional<NodeId> next,
            std::vector<Vec<3>>& out
        ) override;

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
            Player::Rideable rideable;
        };

        static const std::vector<CarriageTypeInfo>& carriage_types();

        enum CarriageType {
            Round, Passenger
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
            engine::Arena::Array<HorseType> horses;
        };

        CarriageType type;
        std::vector<HorseType> horses;

        private:
        engine::Speaker speaker = engine::Speaker(
            engine::Speaker::Space::World, 5.0
        );
        std::shared_ptr<Interactable> interactable = nullptr;
        Player::Rideable rideable;
        AgentState prev_state = AgentState::Travelling;
        f64 last_step_time = 0.0;

        public:
        Carriage(
            CarriageType type, Vec<3> position, StatefulRNG& rng,
            const Settings& settings
        );
        Carriage(
            const Serialized& serialized, const engine::Arena& buffer, 
            const Settings& settings
        );

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

        std::string_view local_name() override {
            return Carriage::carriage_types()
                .at((size_t) this->type).local_name;
        }

        const ui::Background* icon() { 
            return Carriage::carriage_types().at((size_t) this->type).icon;
        }

        void update_rideable(Player& player, Interactables& interactables);

        void update(
            CarriageNetwork& network, engine::Scene& scene, 
            const engine::Window& window, ParticleManager* particles,
            Player& player, Interactables* interactables
        ) override;

        Mat<4> build_transform(
            Vec<3>* position_out = nullptr, 
            f64* pitch_out = nullptr, f64* yaw_out = nullptr
        ) const;

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