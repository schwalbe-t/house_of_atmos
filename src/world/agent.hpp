
#pragma once

#include "complex.hpp"
#include "../renderer.hpp"
#include "../particles.hpp"
#include "../player.hpp"
#include "../interactable.hpp"
#include <algorithm>

namespace houseofatmos::world {

    using namespace houseofatmos::engine::math;


    template<typename NetworkNode>
    struct AgentNetwork {
        using NodeId = NetworkNode::NodeId;
        using NodeIdHash = NetworkNode::NodeIdHash;


        ComplexBank* complexes;
        const Terrain* terrain;
        std::string local_lost_msg;

        AgentNetwork(
            ComplexBank* complexes, const Terrain* terrain, 
            std::string local_lost_msg
        ): complexes(complexes), terrain(terrain), 
            local_lost_msg(std::move(local_lost_msg)) {}

        AgentNetwork(AgentNetwork&& other) noexcept = default;
        AgentNetwork& operator=(AgentNetwork&& other) noexcept = default;


        const TrackPiece& track_piece_at(const NodeId& node_id) const {
            const Terrain::ChunkData& chunk 
                = this->terrain->chunk_at(node_id.chunk_x, node_id.chunk_z);
            return chunk.track_pieces[node_id.piece_i];
        }


        virtual void collect_next_nodes(
            std::optional<NodeId> prev, NodeId node, 
            std::vector<std::pair<NodeId, u64>>& out
        ) = 0;

        virtual u64 node_target_dist(NodeId node, ComplexId target) = 0;

        virtual bool node_at_target(NodeId node, ComplexId target) = 0; 

        virtual void reset() {}

        virtual void update(
            engine::Scene& scene, const engine::Window& window
        ) {
            (void) scene;
            (void) window;
        }

        virtual void render(
            const Vec<3>& observer, f64 draw_distance,
            Renderer& renderer, engine::Scene& scene, 
            const engine::Window& window
        ) {
            (void) observer;
            (void) draw_distance;
            (void) renderer;
            (void) scene;
            (void) window;
        }

        virtual ~AgentNetwork() = default;

    };



    template<typename Network>
    struct AgentPath {
        using NodeId = Network::NodeId;
        using NodeIdHash = Network::NodeIdHash;

        std::vector<NodeId> points;

        private:
        struct NodeSearchState {
            u64 start_dist;
            u64 target_dist;
            std::optional<NodeId> parent;
            bool explored = false;
        };

        using NodeSearchStates = std::unordered_map<
            NodeId, NodeSearchState, NodeIdHash
        >;

        static std::optional<NodeId> cheapest_node(
            const NodeSearchStates& nodes
        ) {
            u64 cheapest_cost = UINT64_MAX;
            std::optional<NodeId> found_node = std::nullopt;
            for(const auto& [node, state]: nodes) {
                if(state.explored) { continue; }
                u64 total_cost = state.start_dist + state.target_dist;
                if(total_cost >= cheapest_cost) { continue; }
                cheapest_cost = total_cost;
                found_node = node;
            }
            return found_node;
        }

        static AgentPath<Network> build_path(
            const NodeSearchStates& nodes, NodeId last
        ) {
            auto path = AgentPath<Network>();
            NodeId current = last;
            for(;;) {
                const NodeSearchState& state = nodes.at(current);
                if(!state.parent.has_value()) { break; }
                path.points.push_back(current);
                current = *state.parent;
            }
            std::reverse(path.points.begin(), path.points.end());
            return path;
        }

        public:
        static std::optional<AgentPath<Network>> find(
            Network& network, NodeId start, ComplexId target,
            std::optional<NodeId> start_parent = std::nullopt
        ) {
            NodeSearchStates nodes;
            nodes[start] = NodeSearchState(
                0, network.node_target_dist(start, target), std::nullopt
            );
            std::vector<std::pair<NodeId, u64>> connected;
            for(;;) {
                std::optional<NodeId> next = cheapest_node(nodes);
                if(!next.has_value()) { return std::nullopt; }
                NodeId current = *next;
                NodeSearchState& current_s = nodes[current];
                current_s.explored = true;
                if(network.node_at_target(current, target)) {
                    return build_path(nodes, current);
                }
                network.collect_next_nodes(
                    current_s.parent.has_value()
                        ? current_s.parent : start_parent, 
                    current, connected
                );
                for(auto [neigh, step_dist]: connected) {
                    u64 new_start_dist = current_s.start_dist + step_dist;
                    if(!nodes.contains(neigh)) {
                        nodes[neigh] = NodeSearchState(
                            new_start_dist,
                            network.node_target_dist(current, target),
                            current
                        );
                    }
                    NodeSearchState& neigh_s = nodes[neigh];
                    bool new_path_shorter = new_start_dist < neigh_s.start_dist;
                    if(new_path_shorter && !neigh_s.explored) {
                        neigh_s.start_dist = new_start_dist;
                        neigh_s.parent = current;
                    }
                }
                connected.clear();
            }
        }

    };



    struct AgentStop {
        enum Action { Load, Unload, Maintain };
        enum Unit { Fixed, Fraction };
        
        ComplexId target;
        Action action;
        union { u32 fixed; f32 fract; } amount;
        Unit unit;
        Item::Type item;
    };

    enum struct AgentState {
        Idle, Travelling, Loading, Lost
    };

    struct SerializedAgent {
        engine::Arena::Array<AgentStop> schedule;
        u64 stop_i;
        engine::Arena::Map<Item::Type, u64> items;
    };

    template<typename Network>
    struct Agent {

        std::vector<AgentStop> schedule;
        u64 stop_i = 0;
        std::unordered_map<Item::Type, u64> items;

        private:
        AgentState state = AgentState::Idle;
        std::optional<AgentPath<Network>> path;
        f64 load_start_time = 0.0;

        public:
        Agent() {}
        Agent(const SerializedAgent& serialized, const engine::Arena& buffer) {
            buffer.copy_into(serialized.schedule, this->schedule);
            this->stop_i = serialized.stop_i;
            buffer.copy_into(serialized.items, this->items);
        }
        Agent(Agent&& other) noexcept = default;
        Agent& operator=(Agent&& other) noexcept = default;

        SerializedAgent serialize(engine::Arena& buffer) const {
            return SerializedAgent(
                buffer.alloc(this->schedule),
                this->stop_i,
                buffer.alloc(this->items)
            );
        }

        virtual Vec<3> current_position(Network& network) = 0;
        
        virtual u64 item_storage_capacity() = 0;

        virtual std::string_view local_name() = 0;

        virtual const ui::Background* icon() { 
            return nullptr; 
        }

        virtual std::optional<AgentPath<Network>> find_path_to(
            Network& network, ComplexId target
        ) = 0;

        virtual void on_network_reset(Network& network) {
            (void) network;
        }

        virtual void update(
            Network& network, engine::Scene& scene, 
            const engine::Window& window, ParticleManager* particles,
            Player& player, Interactables* interactables
        ) {
            (void) network;
            (void) scene;
            (void) window;
            (void) particles;
            (void) player;
            (void) interactables;
        }

        virtual void render(
            Renderer& renderer, Network& network,
            engine::Scene& scene, const engine::Window& window
        ) {
            (void) renderer;
            (void) network;
            (void) scene;
            (void) window;
        }

        virtual ~Agent() = default;

        const AgentStop& next_stop() const {
            if(this->schedule.size() == 0) {
                engine::error("No current target! (schedule.size() == 0)");
            }
            return this->schedule[this->stop_i];
        }
        AgentState current_state() const { return this->state; }
        const std::optional<AgentPath<Network>>& current_path() const { 
            return this->path; 
        }

        void advance_next_stop() {
            this->stop_i += 1;
            this->stop_i = this->stop_i % this->schedule.size();
        }

        void reached_target(const engine::Window& window) {
            this->state = AgentState::Loading;
            this->load_start_time = window.time();
        }

        void travel_to(Network& network, ComplexId target) {
            auto found = this->find_path_to(network, target);
            this->path = found;
            if(!found.has_value()) {
                this->state = AgentState::Lost;
                return;
            }
            this->state = AgentState::Travelling;
        }

        void do_stop_transfer(Network& network, const AgentStop& stop) {
            Complex& complex = network.complexes->get(stop.target);
            u64 complex_has = complex.stored_count(stop.item);
            u64 agent_has = this->items[stop.item];
            u64 fract_rel_to;
            switch(stop.action) {
                case AgentStop::Load: 
                    fract_rel_to = complex_has; break;
                case AgentStop::Unload: 
                    fract_rel_to = agent_has; break;
                case AgentStop::Maintain:
                    fract_rel_to = this->item_storage_capacity(); break;
            }
            u64 expected;
            switch(stop.unit) {
                case AgentStop::Fixed: 
                    expected = (u64) stop.amount.fixed; 
                    break;
                case AgentStop::Fraction: 
                    expected = (u64) ((f64) fract_rel_to * stop.amount.fract); 
                    break;
            }
            AgentStop::Action action = stop.action;
            u64 planned = 0;
            switch(stop.action) {
                case AgentStop::Load:
                    planned = std::min(expected, complex_has);
                    break;
                case AgentStop::Unload:
                    planned = std::min(expected, agent_has);
                    break;
                case AgentStop::Maintain:
                    if(agent_has < expected) {
                        action = AgentStop::Load;
                        planned = std::min(expected - agent_has, complex_has);
                    } else if(agent_has > expected) {
                        action = AgentStop::Unload;
                        planned = agent_has - expected;
                    }
                    break;
            }
            switch(action) {
                case AgentStop::Load: {
                    u64 used_space = 0;
                    for(const auto stack: this->items) {
                        used_space += stack.second;
                    }
                    u64 remaining_space = this->item_storage_capacity()
                        - used_space;
                    u64 transferred = std::min(planned, remaining_space);
                    complex.remove_stored(stop.item, transferred);
                    this->items[stop.item] += transferred;
                    break;
                }
                case AgentStop::Unload: {
                    u64 transferred = complex.add_stored(
                        stop.item, planned, *network.terrain
                    );
                    this->items[stop.item] -= transferred;
                    break;
                }
                case AgentStop::Maintain: break;
            }
        }

        static inline const f64 load_time = 5.0;

        void update_state(Network& network, const engine::Window& window) {
            if(this->schedule.size() >= 1) {
                this->stop_i = this->stop_i % this->schedule.size();
            }
            if(this->schedule.size() < 2) { 
                this->state = AgentState::Idle; 
            }
            switch(this->state) {
                case AgentState::Idle: {
                    this->path = std::nullopt;
                    if(this->schedule.size() >= 2) {
                        this->state = AgentState::Travelling;
                    }
                    break;
                }
                case AgentState::Travelling: {
                    if(!this->path.has_value()) {
                        this->travel_to(network, this->next_stop().target);
                    }
                    break;
                }
                case AgentState::Loading: {
                    this->path = std::nullopt;
                    f64 load_done_time = this->load_start_time + load_time;
                    if(window.time() >= load_done_time) {
                        this->do_stop_transfer(network, this->next_stop());
                        this->advance_next_stop();
                        this->travel_to(network, this->next_stop().target);       
                    }
                    break;
                }
                case AgentState::Lost: {
                    this->path = std::nullopt;
                    break;
                }
            }
        }


        static std::pair<f64, f64> compute_heading_angles(
            const Vec<3>& heading, const Vec<3>& model_heading
        ) {
            Vec<3> vert_heading = Vec<3>(
                model_heading.x(),
                heading.y(),
                model_heading.z()
            );
            f64 pitch_cross = model_heading.y() * vert_heading.z()
                - model_heading.z() * vert_heading.y();
            f64 pitch = atan2(pitch_cross, model_heading.dot(vert_heading));
            f64 yaw_cross = model_heading.x() * heading.z()
                - model_heading.z() * heading.x();
            f64 yaw = atan2(yaw_cross, model_heading.dot(heading));
            return { pitch, yaw };
        }

    };



    template<typename Agent, typename Network>
    struct AgentManager {
        using SerializedAgent = Agent::Serialized;

        struct Serialized {
            engine::Arena::Array<SerializedAgent> agents;
        };

        std::list<Agent> agents;
        Network network;

        AgentManager(Network&& network): network(std::move(network)) {}
        AgentManager(
            Network&& network, 
            const Serialized& serialized, const engine::Arena& buffer,
            const Settings& settings
        ): network(std::move(network)) {
            buffer.copy_into<SerializedAgent, Agent>(
                serialized.agents, this->agents,
                [&](const auto& a) { return Agent(a, buffer, settings); }
            );
        }


        Serialized serialize(engine::Arena& buffer) const {
            return Serialized(buffer.alloc<Agent, SerializedAgent>(
                this->agents,
                [&](const auto& a) { return a.serialize(buffer); }
            ));
        }

        void reset(Toasts* toasts) {
            this->network.reset();
            bool any_became_lost = false;
            for(Agent& agent: this->agents) {
                agent.on_network_reset(this->network);
                if(agent.current_state() == AgentState::Idle) { continue; }
                bool was_lost = agent.current_state() == AgentState::Lost;
                agent.travel_to(this->network, agent.next_stop().target);
                bool is_lost = agent.current_state() == AgentState::Lost;
                any_became_lost |= (!was_lost && is_lost);
            }
            if(any_became_lost && toasts != nullptr) {
                toasts->add_error(this->network.local_lost_msg, {});
            }
        }

        void update(
            engine::Scene& scene, const engine::Window& window,
            ParticleManager* particles,
            Player& player, Interactables* interactables
        ) {
            this->network.update(scene, window);
            for(Agent& agent: this->agents) {
                agent.update_state(this->network, window);
                agent.update(
                    this->network, scene, window, particles, 
                    player, interactables
                );
            }
        }

        void render(
            const Vec<3>& observer, f64 draw_distance,
            Renderer& renderer, engine::Scene& scene, 
            const engine::Window& window
        ) {
            this->network.render(
                observer, draw_distance, renderer, scene, window
            );
            for(Agent& agent: this->agents) {
                Vec<3> position = agent.current_position(this->network);
                f64 distance = (position - observer).len();
                bool is_visible = distance <= draw_distance;
                if(!is_visible) { continue; }
                agent.render(renderer, this->network, scene, window);
            }
        }

    };

}