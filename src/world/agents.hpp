
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

        AgentNetwork(ComplexBank* complexes): complexes(complexes) {}

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

        virtual NodeId closest_node_to(const Vec<3>& position) = 0;

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
        };

        std::vector<Section> sections;


        private:
        std::optional<size_t> next_section_i(size_t after = 0) const {
            size_t sects = this->sections.size();
            size_t sect_i = after;
            while(sect_i < sects && this->sections[sect_i].points.size() == 0) { 
                sect_i += 1; 
            }
            if(sect_i >= sects) { return std::nullopt; }
            return sect_i;
        }

        public:
        void clear() { this->sections.clear(); }
        bool is_empty() const { return !this->next_section_i().has_value(); }

        std::pair<Vec<3>, size_t> after(
            f64 distance, bool* at_end = nullptr
        ) const {
            f64 remaining = distance;
            std::optional<size_t> start_sect_i = this->next_section_i();
            if(!start_sect_i.has_value()) {
                engine::error("Agent used an empty path!");
            }
            size_t sect_i = *start_sect_i;
            Vec<3> position = this->sections[sect_i].points[0];
            for(;;) {
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
                std::optional<size_t> n_sect_i = this->next_section_i(sect_i);
                if(!n_sect_i.has_value()) { 
                    if(at_end != nullptr) { *at_end = true; }
                    return { position, sect_i }; 
                }
                sect_i = *n_sect_i;
            }
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
            NodeId last
        ) {
            NodeId current = last;
            AgentPath<Network> path;
            for(;;) {
                const NodeSearchState& state = nodes.at(current);
                path.sections.push_back(AgentPath<Network>::Section(current));
                if(!state.parent.has_value()) { break; }
                current = *state.parent;
            }
            std::reverse(path.sections.begin(), path.sections.end());
            for(size_t sect_i = 0; sect_i < path.sections.size(); sect_i += 1) {
                AgentPath<Network>::Section& section = path.sections[sect_i];
                std::optional<NodeId> prev = sect_i > 0
                    ? std::optional<NodeId>(path.sections[sect_i - 1].node) 
                    : std::nullopt;
                bool is_last_section = sect_i + 1 >= path.sections.size();
                std::optional<NodeId> next = is_last_section
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
            NodeId start = network.closest_node_to(start_pos);
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
                    return build_path(network, nodes, current);
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
                    if(neigh_s.explored || !new_path_shorter) { continue; }
                    neigh_s.start_dist = new_start_dist;
                    neigh_s.parent = current;
                }
                connected.clear();
            }
        }

    };



    template<typename Network>
    struct Agent {

        struct Stop {
            enum Action { Load, Unload };
            enum Unit { Fixed, Fraction };
            
            ComplexId target;
            Action action;
            union { u32 fixed; f32 fract; } amount;
            Unit unit;
            Item::Type item;
        };

        enum State {
            Idle, Travelling, Loading, Lost
        };

        std::vector<Stop> schedule;
        u64 stop_i = 0;
        Vec<3> position;
        std::unordered_map<Item::Type, u64> items;
        
        private:
        State state = Travelling;
        AgentPath<Network> path;
        f64 distance = 0.0;
        f64 load_start_time = 0.0;

        void do_stop_transfer(Network& network, const Stop& stop) {
            Complex& complex = network.complexes->get(stop.target);
            u64 s_num;
            switch(stop.unit) {
                case Stop::Fixed: s_num = (u64) stop.amount.fixed; break;
                case Stop::Fraction: s_num = (u64) stop.amount.fract; break;
            }
            u64 p_num;
            switch(stop.action) {
                case Stop::Load: p_num = complex.stored_count(stop.item); break;
                case Stop::Unload: p_num = this->items[stop.item]; break;
            }
            u64 planned = std::min(s_num, p_num);
            switch(stop.action) {
                case Stop::Load: {
                    u64 remaining_space = this->item_storage_capacity()
                        - this->stored_item_count();
                    u64 transferred = std::min(planned, remaining_space);
                    complex.remove_stored(stop.item, transferred);
                    this->items[stop.item] += transferred;
                    break;
                }
                case Stop::Unload: {
                    u64 transferred = planned;
                    this->items[stop.item] -= transferred;
                    complex.add_stored(stop.item, transferred);
                }
            }
        }

        public:
        Agent() {}
        Agent(Agent&& other) noexcept = default;
        Agent& operator=(Agent&& other) noexcept = default;

        virtual f64 current_speed(Network& network) = 0;

        virtual u64 item_storage_capacity() = 0;

        u64 stored_item_count() const {
            u64 total = 0;
            for(const auto [item, count]: this->items) {
                total += count;
            }
            return total;
        }

        State current_state() const { return this->state; }

        void reset_path() { this->path.clear(); }

        static inline const f64 load_time = 5.0;

        void update_state(Network& network, const engine::Window& window) {
            if(this->schedule.size() == 0) { this->state = State::Idle; }
            if(this->schedule.size() >= 1) {
                this->stop_i = this->stop_i % this->schedule.size();
            }
            switch(this->state) {
                case Idle: {
                    if(this->schedule.size() > 0) { 
                        this->path.clear();
                        this->state = Travelling;
                    }
                    break;
                }
                case Travelling: {
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
                        this->state = Loading; 
                    }
                    break;
                }
                case Loading: {
                    f64 load_done_time = this->load_start_time + load_time;
                    if(window.time() >= load_done_time) {
                        const Stop& stop = this->schedule[this->stop_i];
                        this->do_stop_transfer(network, stop);
                        this->stop_i += 1;
                        this->path.clear();
                        this->state = Travelling;
                    }
                    break;
                }
                case Lost: break;
            }
        }

        bool find_path(Network& network) {
            if(this->state == Idle || this->state == Loading) { return false; }
            ComplexId target = this->schedule[this->stop_i].target;
            std::optional<AgentPath<Network>> found 
                = AgentPath<Network>::find(network, this->position, target);
            if(!found.has_value()) {
                this->state = Lost;
                return false;
            }
            this->state = Travelling;
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

        void find_paths() {
            for(Agent& agent: this->agents) {
                agent.find_path(this->network);
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