
#pragma once

#include "agent.hpp"
#include "terrain.hpp"
#include <algorithm>

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
            std::vector<const Train*> queue; // FIFO ([0] is next)
            u64 size = 0; // number of pieces that belong to the block
        
            Block(Type type = Block::Simple): type(type) {}

            bool in_queue(const Train* train) {
                return std::find(this->queue.begin(), this->queue.end(), train)
                    != this->queue.end();
            }
            void await(const Train* train) { 
                if(this->in_queue(train) || this->owner == train) { return; }
                this->queue.push_back(train); 
            }
            bool may_take(const Train* train) {
                return this->owner == nullptr
                    && this->queue.size() >= 1
                    && this->queue[0] == train;
            }
            void take(const Train* train) {
                if(!this->may_take(train)) { return; }
                this->owner = train;
                this->queue.erase(this->queue.begin());
            }
            void release(const Train* train) {
                if(this->owner != train) { return; }
                this->owner = nullptr;
            }
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

            void render(Renderer& renderer, engine::Scene& scene) {
                engine::Model& model = scene.get(TrackNetwork::semaphore);
                Mat<4> inst = Mat<4>::translate(this->position)
                    * Mat<4>::rotate_y(this->rotation);
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
        
        Terrain* terrain;
        StatefulRNG rng;
        std::list<Block> blocks;
        std::list<Signal> signals;
        std::unordered_map<NodeId, Node, NodeIdHash> graph;

        TrackNetwork(Terrain* terrain, ComplexBank* complexes):
            AgentNetwork(complexes, "toast_train_lost"), terrain(terrain) {}

        TrackNetwork(TrackNetwork&& other) noexcept = default;
        TrackNetwork& operator=(TrackNetwork&& other) noexcept = default;


        private:
        void find_connections(NodeId node, Node& node_data);
        void assign_to_blocks(NodeId node, Block* previous = nullptr);
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
            f64 speed;
            u64 cost;
        };

        static const std::vector<LocomotiveTypeInfo>& locomotive_types();

        enum LocomotiveType {
            Basic, Small, Tram
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
        std::vector<CarState> cars;
        std::vector<OwnedBlock> owning_blocks;
        AgentState prev_state = AgentState::Idle;
        f64 last_chugga_time = 0.0;
        f64 last_back_dist = 0.0;

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

        f64 wait_point_distance(TrackNetwork& network) const {
            const auto& sections = this->current_path().sections;
            size_t section_i = this->current_path()
                .after(this->front_path_dist()).second + 1;
            f64 distance = 0.0;
            for(; section_i < sections.size(); section_i += 1) {
                const auto& section = sections[section_i];
                const TrackNetwork::Node& node = network.graph.at(section.node);
                if(node.block->owner != this) { return distance; }
                distance += section.length();
            }
            return INFINITY;
        }


        f64 current_speed(TrackNetwork& network) override {
            f64 wait_after = this->wait_point_distance(network);
            f64 top_speed = Train::locomotive_types()
                .at((size_t) this->loco_type).speed;
            return std::min(wait_after, top_speed);
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

        void manage_block_ownership(TrackNetwork& network);

        void update(
            TrackNetwork& network, engine::Scene& scene, 
            const engine::Window& window, ParticleManager* particles
        ) override;

        Vec<3> find_heading(size_t car_idx) const;

        void render(
            Renderer& renderer, TrackNetwork& network,
            engine::Scene& scene, const engine::Window& window
        ) override;

    };


    using TrainManager = AgentManager<Train, TrackNetwork>;

}