
#pragma once

#include "agent.hpp"
#include "terrain.hpp"
#include <algorithm>
#include <unordered_set>

namespace houseofatmos::world {

    struct Train;



    struct TrackNetworkNode {
        using NodeId = const TrackPiece*;
        struct NodeIdHash {
            std::size_t operator()(const NodeId& p) const { 
                return (size_t) p; 
            }
        };
    };

    struct TrackNetwork: AgentNetwork<TrackNetworkNode> {
        
        static inline const engine::Model::LoadArgs semaphore = {
            "res/trains/semaphore.glb", Renderer::model_attribs,
            engine::FaceCulling::Disabled
        };


        struct Block {
            enum Type { Simple, Conflict };

            Type type;
            const Train* owner = nullptr;
            u64 size = 0; // number of pieces that belong to the block
        
            Block(Type type = Block::Simple): type(type) {}
        };

        struct Signal {
            enum State { Rising, Proceed, Falling, Danger };
            
            inline static f64 state_duration(State state) {
                switch(state) {
                    case Proceed: case Danger: return INFINITY;
                    case Rising: return 12.0 / 24.0;
                    case Falling: return 20.0 / 24.0;
                }
                engine::error("Unhandled 'Signal::State'!");         
            }

            inline static State next_state(State state) {
                switch(state) {
                    case Rising: return Proceed;
                    case Proceed: return Falling;
                    case Falling: return Danger;
                    case Danger: return Rising;
                }       
                engine::error("Unhandled 'Signal::State'!");         
            }

            inline static std::string state_anim_name(State state) {
                switch(state) {
                    case Rising: return "to_proceed";
                    case Proceed: return "at_proceed";
                    case Falling: return "to_danger";
                    case Danger: return "at_danger";
                }
                engine::error("Unhandled 'Signal::State'!");
            }

            Block* from;
            Block* to;
            
            Vec<3> position;
            f64 rotation;
            State state = Danger;
            f64 timer = 0.0;

            Signal(Block* from, Block* to, Vec<3> position, f64 rotation):
                from(from), to(to), position(position), rotation(rotation) {}

            State desired_state() {
                bool clear = this->to->owner == this->from->owner;
                if(this->to->type == Block::Conflict) {
                    clear &= this->from->owner != nullptr;
                }
                if(this->to->type == Block::Simple) {
                    clear |= this->to->owner == nullptr;
                }
                return clear? Proceed : Danger;
            }

            void update(const engine::Window& window) {
                this->timer += window.delta_time();
                State desired = this->desired_state();
                bool start_rising = desired == Proceed
                    && this->state != Proceed 
                    && this->state != Rising;
                if(start_rising) { 
                    this->state = Rising; 
                    this->timer = 0.0;
                }
                bool start_falling = desired == Danger
                    && this->state != Danger 
                    && this->state != Falling;
                if(start_falling) { 
                    this->state = Falling; 
                    this->timer = 0.0;
                }
                if(this->timer >= Signal::state_duration(this->state)) {
                    this->state = Signal::next_state(this->state);
                    this->timer = 0.0;
                }
            }

            void render(
                TrackNetwork& network, Renderer& renderer, engine::Scene& scene
            ) {
                engine::Model& model = scene.get(TrackNetwork::semaphore);
                Vec<3> scale = network.settings->signal_side_left
                    ? Vec<3>(1, 1, 1) : Vec<3>(-1, 1, 1);
                Mat<4> inst = Mat<4>::translate(this->position)
                    * Mat<4>::rotate_y(this->rotation)
                    * Mat<4>::scale(scale);
                const engine::Animation& animation = model
                    .animation(Signal::state_anim_name(this->state));
                renderer.render(
                    model, std::array { inst }, &animation, this->timer
                );
            }
        };

        struct Node {
            u64 chunk_x, chunk_z;
            std::vector<NodeId> connected_low;
            std::vector<NodeId> connected_high;
            Block* block = nullptr;
        };
        
        const Settings* settings;
        Terrain* terrain;
        StatefulRNG rng;
        std::list<Block> blocks;
        std::vector<Signal> signals;
        std::unordered_map<NodeId, Node, NodeIdHash> graph;

        TrackNetwork(
            const Settings* settings, Terrain* terrain, ComplexBank* complexes
        ): AgentNetwork(complexes, terrain, "toast_train_lost"), 
            settings(settings), terrain(terrain) {}

        TrackNetwork(TrackNetwork&& other) noexcept = default;
        TrackNetwork& operator=(TrackNetwork&& other) noexcept = default;


        private:
        void find_connections(NodeId node, Node& node_data);
        void assign_nodes_to_blocks(NodeId node, Block* previous = nullptr);
        void create_signals(NodeId node);
        public:
        void reload() override;


        void collect_next_nodes(
            std::optional<NodeId> prev, NodeId node, 
            std::vector<std::pair<NodeId, u64>>& out
        ) override;

        u64 node_target_dist(NodeId node, ComplexId target) override;

        bool node_at_target(NodeId node, ComplexId target) override;

        void collect_node_points(
            std::optional<NodeId> prev, NodeId node, std::optional<NodeId> next,
            std::vector<Vec<3>>& out
        ) override;

        std::optional<NodeId> closest_node_to(const Vec<3>& position) override;


        void update(
            engine::Scene& scene, const engine::Window& window
        ) override;

        void render(
            const Vec<3>& observer, f64 draw_distance,
            Renderer& renderer, engine::Scene& scene, 
            const engine::Window& window
        ) override;

    };



    struct Train: Agent<TrackNetwork> {

        struct Car {
            engine::Model::LoadArgs model;
            Vec<3> model_heading;
            f64 length;
            f64 front_axle, back_axle;
            f64 wheel_radius;
            u64 capacity;
        };

        static inline const u64 train_car_cost = 250;

        struct LocomotiveTypeInfo {
            std::string_view local_name;
            const ui::Background* icon;
            std::vector<Car> loco_cars;
            const Car& car_type;
            Vec<3> smoke_origin;
            f64 whistle_pitch;
            u64 max_car_count;
            f64 acceleration;
            f64 braking_distance;
            f64 top_speed;
            u64 cost; 
            Player::Rideable rideable;
        };

        static const std::vector<LocomotiveTypeInfo>& locomotive_types();

        enum LocomotiveType {
            Basic, Small, Tram
        };


        static void load_resources(engine::Scene& scene) {
            scene.load(TrackNetwork::semaphore);
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

        struct OwnedBlock {
            TrackNetwork::Block* block;
            bool justified;
        };

        LocomotiveType loco_type;
        u64 car_count;

        private:
        engine::Speaker speaker = engine::Speaker(
            engine::Speaker::Space::World, 5.0
        );
        std::shared_ptr<Interactable> interactable = nullptr;
        Player::Rideable rideable;
        std::vector<OwnedBlock> owning_blocks;
        AgentState prev_state = AgentState::Idle;
        f64 last_chugga_time = 0.0;
        f64 velocity = 0.0;

        public:
        Train(
            LocomotiveType loco_type, Vec<3> position, 
            const Settings& settings
        );
        Train(
            const Serialized& serialized, const engine::Arena& buffer,
            const Settings& settings
        );

        Train(Train&& other) noexcept = default;
        Train& operator=(Train&& other) noexcept = default;

        Serialized serialize(engine::Arena& buffer) const;


        static inline const f64 car_padding = 0.5;

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

        f64 wait_point_distance(TrackNetwork& network) const;


        f64 current_speed(TrackNetwork& network) override {
            (void) network;
            f64 await_signal_after = this->wait_point_distance(network);
            if(await_signal_after < 1.0) { return 0.0; }
            const LocomotiveTypeInfo& loco_info = Train::locomotive_types()
                .at((size_t) this->loco_type);
            f64 len = this->length();
            if(this->current_path_dist() < len) { return loco_info.top_speed; }
            return this->velocity;
        }

        u64 item_storage_capacity() override {
            const LocomotiveTypeInfo& loco_info = Train::locomotive_types()
                .at((size_t) this->loco_type);
            u64 total = 0;
            for(const Car& car: loco_info.loco_cars) {
                total += car.capacity;
            }
            total += this->car_count * loco_info.car_type.capacity;
            return total;
        }

        std::string_view local_name() override {
            return Train::locomotive_types()
                .at((size_t) this->loco_type).local_name;
        }

        const ui::Background* icon() { 
            return Train::locomotive_types().at((size_t) this->loco_type).icon;
        }

        Mat<4> build_car_transform(
            size_t car_idx, 
            Vec<3>* position_out = nullptr, 
            f64* pitch_out = nullptr, f64* yaw_out = nullptr
        ) const;

        void release_unjustified_blocks(TrackNetwork& network);
        void take_next_blocks(TrackNetwork& network);

        void on_new_path(TrackNetwork& network) override;
        void on_network_reset(TrackNetwork& network) override;

        void update_velocity(
            const engine::Window& window, TrackNetwork& network
        );

        void update_rideable(Player& player, Interactables& interactables);

        void update(
            TrackNetwork& network, engine::Scene& scene, 
            const engine::Window& window, ParticleManager* particles,
            Player& player, Interactables* interactables
        ) override;

        void render(
            Renderer& renderer, TrackNetwork& network,
            engine::Scene& scene, const engine::Window& window
        ) override;

    };


    using TrainManager = AgentManager<Train, TrackNetwork>;

}