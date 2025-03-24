
#pragma once

#include "complex.hpp"
#include "../renderer.hpp"
#include <algorithm>

namespace houseofatmos::world {

    using namespace houseofatmos::engine::math;


    template<typename NetworkNode>
    struct AgentNetwork {
        using NodeId = NetworkNode::NodeId;
        using NodeIdHash = NetworkNode::NodeIdHash;


        ComplexBank* complexes;
        std::string local_lost_msg;

        AgentNetwork(
            ComplexBank* complexes, std::string local_lost_msg
        ): complexes(complexes), local_lost_msg(std::move(local_lost_msg)) {}

        AgentNetwork(AgentNetwork&& other) noexcept = default;
        AgentNetwork& operator=(AgentNetwork&& other) noexcept = default;


        virtual void collect_next_nodes(
            NodeId node, std::vector<std::pair<NodeId, u64>>& out
        ) = 0;

        virtual u64 node_target_dist(NodeId node, ComplexId target) = 0;

        virtual bool node_at_target(NodeId node, ComplexId target) = 0; 

        virtual void collect_node_points(
            std::optional<NodeId> prev, NodeId node, std::optional<NodeId> next,
            std::vector<Vec<3>>& out
        ) = 0;

        virtual std::optional<NodeId> closest_node_to(
            const Vec<3>& position
        ) = 0;

        virtual void reload() {}

        virtual void update(
            engine::Scene& scene, const engine::Window& window
        ) {
            (void) scene;
            (void) window;
        }

        virtual ~AgentNetwork() = default;

    };



    template<typename Network>
    struct AgentPath {
        using NodeId = Network::NodeId;
        using NodeIdHash = Network::NodeIdHash;

        struct Section {
            NodeId node;
            std::vector<Vec<3>> points; 

            Section(NodeId node): node(node) {}
        };

        Vec<3> start;
        std::vector<Section> sections;

        public:
        void clear() { this->sections.clear(); }
        bool is_empty() const { return this->sections.size() == 0; }

        std::pair<Vec<3>, size_t> after(
            f64 distance, bool* at_end = nullptr
        ) const {
            size_t sect_c = this->sections.size();
            f64 remaining = distance;
            Vec<3> position = this->start;
            for(size_t sect_i = 0; sect_i < sect_c; sect_i += 1) {
                const Section& section = this->sections[sect_i];
                for(const Vec<3>& point: section.points) {
                    Vec<3> step = point - position;
                    f64 step_len = step.len();
                    f64 step_progress = std::min(remaining / step_len, 1.0);
                    position += step * step_progress;
                    if(remaining <= step_len) { 
                        if(at_end != nullptr) { *at_end = false; }
                        return { position, sect_i }; 
                    }
                    remaining -= step_len;
                }
            }
            if(at_end != nullptr) { *at_end = true; }
            return { position, sect_c - 1 }; 
        }


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
            Network& network, const NodeSearchStates& nodes, 
            Vec<3> start_pos, NodeId last
        ) {
            auto path = AgentPath<Network>();
            path.start = start_pos;
            NodeId current = last;
            for(;;) {
                const NodeSearchState& state = nodes.at(current);
                if(!state.parent.has_value()) { break; }
                path.sections.push_back(AgentPath<Network>::Section(current));
                current = *state.parent;
            }
            std::reverse(path.sections.begin(), path.sections.end());
            for(size_t sect_i = 0; sect_i < path.sections.size(); sect_i += 1) {
                AgentPath<Network>::Section& section = path.sections[sect_i];
                std::optional<NodeId> prev = sect_i >= 1
                    ? std::optional<NodeId>(path.sections[sect_i - 1].node)
                    : std::nullopt;
                std::optional<NodeId> next = sect_i + 1 < path.sections.size()
                    ? std::optional<NodeId>(path.sections[sect_i + 1].node)
                    : std::nullopt;
                network.collect_node_points(
                    prev, section.node, next, section.points
                );
            }
            return path;
        }

        public:
        static std::optional<AgentPath<Network>> find(
            Network& network, Vec<3> start_pos, ComplexId target
        ) {
            std::optional<NodeId> start = network.closest_node_to(start_pos);
            if(!start.has_value()) { return std::nullopt; }
            NodeSearchStates nodes;
            nodes[*start] = NodeSearchState(
                0, network.node_target_dist(*start, target), std::nullopt
            );
            std::vector<std::pair<NodeId, u64>> connected;
            for(;;) {
                std::optional<NodeId> next = cheapest_node(nodes);
                if(!next.has_value()) { return std::nullopt; }
                NodeId current = *next;
                NodeSearchState& current_s = nodes[current];
                current_s.explored = true;
                if(network.node_at_target(current, target)) {
                    return build_path(network, nodes, start_pos, current);
                }
                network.collect_next_nodes(current, connected);
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
        enum Action { Load, Unload };
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

    struct AbstractAgent {
        struct Impl {
            const AgentState& (*state)(void*);
            void (*reset_path)(void*);
            std::vector<AgentStop>& (*schedule)(void*);
            size_t& (*stop_i)(void*);
            Vec<3>& (*position)(void*);
            std::unordered_map<Item::Type, u64>& (*items)(void*);
        };

        void* data;
        const Impl* impl;

        const AgentState& state() const { 
            return this->impl->state(this->data); 
        };
        void reset_path() const {
            this->impl->reset_path(this->data);
        } 
        std::vector<AgentStop>& schedule() const { 
            return this->impl->schedule(this->data); 
        };
        size_t& stop_i() const {
            return this->impl->stop_i(this->data);
        };
        Vec<3>& position() const {
            return this->impl->position(this->data);
        }
        std::unordered_map<Item::Type, u64>& items() const {
            return this->impl->items(this->data);
        }
    };

    struct SerializedAgent {
        u64 stop_count, stop_offset;
        u64 stop_i;
        Vec<3> position;
        u64 items_count, items_offset;
    };

    template<typename Network>
    struct Agent {

        std::vector<AgentStop> schedule;
        u64 stop_i = 0;
        Vec<3> position;
        std::unordered_map<Item::Type, u64> items;
        
        private:
        AgentState state = AgentState::Travelling;
        AgentPath<Network> path;
        f64 distance = 0.0;
        f64 load_start_time = 0.0;

        static inline const AbstractAgent::Impl abstract_impl = {
            +[](void* d) -> const AgentState& { 
                return ((Agent<Network>*) d)->state; 
            },
            +[](void* d) { ((Agent<Network>*) d)->reset_path(); },
            +[](void* d) -> std::vector<AgentStop>& { 
                return ((Agent<Network>*) d)->schedule; 
            },
            +[](void* d) -> size_t& { return ((Agent<Network>*) d)->stop_i; },
            +[](void* d) -> Vec<3>& { return ((Agent<Network>*) d)->position; },
            +[](void* d) -> std::unordered_map<Item::Type, u64>& { 
                return ((Agent<Network>*) d)->items; 
            }
        };

        void do_stop_transfer(Network& network, const AgentStop& stop) {
            Complex& complex = network.complexes->get(stop.target);
            u64 p_num;
            switch(stop.action) {
                case AgentStop::Load: 
                    p_num = complex.stored_count(stop.item); break;
                case AgentStop::Unload: 
                    p_num = this->items[stop.item]; break;
            }
            u64 s_num;
            switch(stop.unit) {
                case AgentStop::Fixed: 
                    s_num = (u64) stop.amount.fixed; break;
                case AgentStop::Fraction: 
                    s_num = (u64) ((f64) p_num * stop.amount.fract); break;
            }
            u64 planned = std::min(s_num, p_num);
            switch(stop.action) {
                case AgentStop::Load: {
                    u64 remaining_space = this->item_storage_capacity()
                        - this->stored_item_count();
                    u64 transferred = std::min(planned, remaining_space);
                    complex.remove_stored(stop.item, transferred);
                    this->items[stop.item] += transferred;
                    break;
                }
                case AgentStop::Unload: {
                    u64 transferred = planned;
                    this->items[stop.item] -= transferred;
                    complex.add_stored(stop.item, transferred);
                }
            }
        }

        public:
        Agent() {}
        Agent(const SerializedAgent& serialized, const engine::Arena& buffer) {
            buffer.copy_array_at_into(
                serialized.stop_offset, serialized.stop_count, this->schedule
            );
            this->stop_i = serialized.stop_i;
            this->position = serialized.position;
            buffer.copy_map_at_into(
                serialized.items_offset, serialized.items_count, this->items
            );
        }
        Agent(Agent&& other) noexcept = default;
        Agent& operator=(Agent&& other) noexcept = default;
        
        SerializedAgent serialize(engine::Arena& buffer) const {
            return SerializedAgent(
                this->schedule.size(), buffer.alloc_array(this->schedule),
                this->stop_i,
                this->position,
                this->items.size(), buffer.alloc_map(this->items)
            );
        }

        virtual f64 current_speed(Network& network) = 0;

        virtual u64 item_storage_capacity() = 0;

        u64 stored_item_count() const {
            u64 total = 0;
            for(const auto [item, count]: this->items) {
                total += count;
            }
            return total;
        }

        AgentState current_state() const { return this->state; }
        const AgentPath<Network>& current_path() const { return this->path; }
        f64 current_path_dist() const { return this->distance; }
        void reset_path() {
            this->path.clear(); 
            this->distance = 0.0;
        }

        AbstractAgent as_abstract() {
            return AbstractAgent((void*) this, &Agent<Network>::abstract_impl);
        }

        static inline const f64 load_time = 5.0;

        void update_state(Network& network, const engine::Window& window) {
            if(this->schedule.size() == 0) { this->state = AgentState::Idle; }
            if(this->schedule.size() >= 1) {
                this->stop_i = this->stop_i % this->schedule.size();
            }
            switch(this->state) {
                case AgentState::Idle: {
                    if(this->schedule.size() > 0) { 
                        this->path.clear();
                        this->state = AgentState::Travelling;
                    }
                    break;
                }
                case AgentState::Travelling: {
                    if(this->path.is_empty()) { 
                        if(!this->find_path(network)) { break; } 
                    }
                    this->distance += window.delta_time() 
                        * this->current_speed(network);
                    bool is_at_end;
                    this->position = this->path
                        .after(this->distance, &is_at_end).first;
                    if(is_at_end) { 
                        this->load_start_time = window.time();
                        this->state = AgentState::Loading; 
                    }
                    break;
                }
                case AgentState::Loading: {
                    f64 load_done_time = this->load_start_time + load_time;
                    if(window.time() >= load_done_time) {
                        const AgentStop& stop = this->schedule[this->stop_i];
                        this->do_stop_transfer(network, stop);
                        this->stop_i += 1;
                        this->path.clear();
                        this->state = AgentState::Travelling;
                    }
                    break;
                }
                case AgentState::Lost: break;
            }
        }

        bool find_path(Network& network) {
            if(this->schedule.size() == 0) { this->state = AgentState::Idle; }
            bool requires_path = this->state == AgentState::Travelling 
                || this->state == AgentState::Lost;
            if(!requires_path) { return false; }
            ComplexId target = this->schedule[this->stop_i].target;
            std::optional<AgentPath<Network>> found 
                = AgentPath<Network>::find(network, this->position, target);
            if(!found.has_value()) {
                this->state = AgentState::Lost;
                return false;
            }
            this->state = AgentState::Travelling;
            this->path = *found;
            this->distance = 0.0;
            return true;
        }

        virtual void update(
            Network& network, engine::Scene& scene, const engine::Window& window
        ) {
            (void) network;
            (void) scene;
            (void) window;
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
            u64 agent_count, agent_offset;
        };

        std::vector<Agent> agents;
        Network network;

        AgentManager(Network&& network): network(std::move(network)) {}
        AgentManager(
            Network&& network, 
            const Serialized& serialized, const engine::Arena& buffer
        ): network(std::move(network)) {
            std::vector<SerializedAgent> agents;
            buffer.copy_array_at_into(
                serialized.agent_offset, serialized.agent_count, agents
            );
            this->agents.reserve(agents.size());
            for(const SerializedAgent& agent: agents) {
                this->agents.push_back(Agent(agent, buffer));
            }
        }


        Serialized serialize(engine::Arena& buffer) const {
            std::vector<SerializedAgent> agents;
            agents.reserve(this->agents.size());
            for(const Agent& agent: this->agents) {
                agents.push_back(agent.serialize(buffer));
            }
            return Serialized(agents.size(), buffer.alloc_array(agents));
        }

        void find_paths(Toasts* toasts) {
            this->network.reload();
            bool any_became_lost = false;
            for(Agent& agent: this->agents) {
                bool was_lost = agent.current_state() == AgentState::Lost;
                agent.find_path(this->network);
                bool is_lost = agent.current_state() == AgentState::Lost;
                any_became_lost |= (!was_lost && is_lost);
            }
            if(any_became_lost && toasts != nullptr) {
                toasts->add_error(this->network.local_lost_msg, {});
            }
        }

        void update(engine::Scene& scene, const engine::Window& window) {
            this->network.update(scene, window);
            for(Agent& agent: this->agents) {
                agent.update_state(this->network, window);
                agent.update(this->network, scene, window);
            }
        }

        void render(
            const Vec<3>& observer, f64 draw_distance,
            Renderer& renderer, engine::Scene& scene, 
            const engine::Window& window
        ) {
            for(Agent& agent: this->agents) {
                f64 distance = (agent.position - observer).len();
                bool is_visible = distance <= draw_distance;
                if(!is_visible) { continue; }
                agent.render(renderer, this->network, scene, window);
            }
        }

    };

}