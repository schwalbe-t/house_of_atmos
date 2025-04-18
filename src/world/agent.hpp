
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


        virtual void collect_next_nodes(
            std::optional<NodeId> prev, NodeId node, 
            std::vector<std::pair<NodeId, u64>>& out
        ) = 0;

        virtual u64 node_target_dist(NodeId node, ComplexId target) = 0;

        virtual bool node_at_target(NodeId node, ComplexId target) = 0; 

        virtual void collect_node_points(
            std::optional<NodeId> prev, NodeId node, std::optional<NodeId> next,
            std::vector<Vec<3>>& out
        ) = 0;

        virtual std::vector<NodeId> closest_nodes_to(
            const Vec<3>& position
        ) = 0;

        virtual void reload() {}

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

        struct Section {
            NodeId node;
            std::vector<Vec<3>> points; 

            Section(NodeId node): node(node) {}

            f64 length(std::optional<Vec<3>> prev = std::nullopt) const {
                if(this->points.size() == 0) { return 0.0; }
                Vec<3> previous = prev.has_value()? *prev : this->points[0];
                size_t i = prev.has_value()? 0 : 1;
                f64 sum = 0.0;
                for(; i < this->points.size(); i += 1) {
                    Vec<3> point = this->points[i];
                    sum += (point - previous).len();
                    previous = point;
                }
                return sum;
            }
        };

        Vec<3> start;
        std::vector<Section> sections;

        std::pair<Vec<3>, size_t> after(f64 distance) const {
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
                        return { position, sect_i }; 
                    }
                    remaining -= step_len;
                }
            }
            return { position, sect_c - 1 }; 
        }

        f64 section_distance(size_t section_i) const {
            f64 sum = 0.0;
            Vec<3> last_pos = this->start;
            for(size_t sect_i = 0; sect_i < section_i; sect_i += 1) {
                const Section& section = this->sections[sect_i];
                sum += section.length(last_pos);
                if(section.points.size() >= 1) {
                    last_pos = section.points.back();
                }
            }
            return sum;
        }

        f64 length() const {
            return this->section_distance(this->sections.size());
        }

        void append(const AgentPath& other) {
            this->sections.insert(
                this->sections.end(),
                other.sections.begin(), other.sections.end()
            );
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
            std::vector<NodeId> starts = network.closest_nodes_to(start_pos);
            NodeSearchStates nodes;
            for(NodeId start: starts) {
                nodes[start] = NodeSearchState(
                    0, network.node_target_dist(start, target), std::nullopt
                );
            }
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
                network.collect_next_nodes(
                    current_s.parent, current, connected
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

    struct AbstractAgent {
        struct Impl {
            const AgentState& (*state)(void*);
            void (*reset_path)(void*);
            std::vector<AgentStop>& (*schedule)(void*);
            size_t& (*stop_i)(void*);
            Vec<3>& (*position)(void*);
            std::unordered_map<Item::Type, u64>& (*items)(void*);
            u64 (*item_storage_capacity)(void*);
            std::string_view (*local_name)(void*);
            const ui::Background* (*icon)(void*);
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
        u64 item_storage_capacity() const {
            return this->impl->item_storage_capacity(this->data);
        }
        std::string_view local_name() const {
            return this->impl->local_name(this->data);
        }
        const ui::Background* icon() const {
            return this->impl->icon(this->data);
        }

        u64 stored_item_count() const {
            u64 total = 0;
            for(const auto [item, count]: this->items()) {
                total += count;
            }
            return total;
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
        bool has_path = false;
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
            },
            +[](void* d) -> u64 { 
                return ((Agent<Network>*) d)->item_storage_capacity(); 
            },
            +[](void* d) -> std::string_view { 
                return ((Agent<Network>*) d)->local_name(); 
            },
            +[](void* d) -> const ui::Background* { 
                return ((Agent<Network>*) d)->icon(); 
            }
        };

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
                    u64 remaining_space = this->item_storage_capacity()
                        - this->stored_item_count();
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

        public:
        Agent(Vec<3> position) {
            this->position = position;
            this->path.start = position;
        }
        Agent(const SerializedAgent& serialized, const engine::Arena& buffer) {
            buffer.copy_array_at_into(
                serialized.stop_offset, serialized.stop_count, this->schedule
            );
            this->stop_i = serialized.stop_i;
            this->position = serialized.position;
            this->path.start = serialized.position;
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

        virtual std::string_view local_name() = 0;

        virtual const ui::Background* icon() { 
            return nullptr; 
        }

        virtual bool at_path_end() {
            return this->distance >= this->path.length();
        }

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
            this->has_path = false;
            this->path = AgentPath<Network>();
            this->path.start = this->position;
            this->distance = 0.0;
        }

        AbstractAgent as_abstract() {
            return AbstractAgent((void*) this, &Agent<Network>::abstract_impl);
        }

        static inline const f64 load_time = 5.0;
        static inline const u64 preserved_section_c = 10;

        void update_state(Network& network, const engine::Window& window) {
            if(this->schedule.size() == 0) { this->state = AgentState::Idle; }
            if(this->schedule.size() >= 1) {
                this->stop_i = this->stop_i % this->schedule.size();
            }
            switch(this->state) {
                case AgentState::Idle: {
                    if(this->schedule.size() > 0) { 
                        this->reset_path();
                        this->state = AgentState::Travelling;
                    }
                    break;
                }
                case AgentState::Travelling: {
                    if(!this->has_path) { 
                        if(!this->find_path(network)) { break; } 
                    }
                    this->distance += window.delta_time() 
                        * this->current_speed(network);
                    this->position = this->path.after(this->distance).first;
                    if(this->at_path_end()) {
                        this->load_start_time = window.time();
                        this->state = AgentState::Loading; 
                    }
                    break;
                }
                case AgentState::Loading: {
                    // reduce path length to a maximum of N segments
                    if(this->path.sections.size() > preserved_section_c) {
                        f64 remaining = this->path.length() - this->distance;
                        this->path.sections.erase(
                            this->path.sections.begin(), 
                            this->path.sections.end() - preserved_section_c
                        );
                        this->distance = this->path.length() - remaining;
                    }
                    f64 load_done_time = this->load_start_time + load_time;
                    if(window.time() >= load_done_time) {
                        const AgentStop& stop = this->schedule[this->stop_i];
                        this->do_stop_transfer(network, stop);
                        this->stop_i += 1;
                        this->state = AgentState::Travelling;
                        this->has_path = false;
                    }
                    break;
                }
                case AgentState::Lost: break;
            }
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

        virtual void on_new_path(Network& network) {
            (void) network;
        }

        virtual void on_network_reset(Network& network) {
            (void) network;
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

        bool find_path(Network& network) {
            if(this->schedule.size() == 0) { this->state = AgentState::Idle; }
            bool requires_path = this->state == AgentState::Travelling 
                || this->state == AgentState::Lost;
            if(!requires_path) { return false; }
            ComplexId target = this->schedule[this->stop_i].target;
            std::optional<AgentPath<Network>> found = AgentPath<Network>::find(
                network, this->path.after(this->path.length()).first, target
            );
            if(!found.has_value()) {
                this->state = AgentState::Lost;
                return false;
            }
            this->path.append(*found);
            this->state = AgentState::Travelling;
            this->has_path = true;
            this->on_new_path(network);
            return true;
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
            u64 agent_count, agent_offset;
        };

        std::list<Agent> agents;
        Network network;

        AgentManager(Network&& network): network(std::move(network)) {}
        AgentManager(
            Network&& network, 
            const Serialized& serialized, const engine::Arena& buffer,
            const Settings& settings
        ): network(std::move(network)) {
            std::vector<SerializedAgent> agents;
            buffer.copy_array_at_into(
                serialized.agent_offset, serialized.agent_count, agents
            );
            for(const SerializedAgent& agent: agents) {
                this->agents.push_back(Agent(agent, buffer, settings));
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
                agent.on_network_reset(network);
                bool was_lost = agent.current_state() == AgentState::Lost;
                agent.reset_path();
                agent.find_path(this->network);
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
                f64 distance = (agent.position - observer).len();
                bool is_visible = distance <= draw_distance;
                if(!is_visible) { continue; }
                agent.render(renderer, this->network, scene, window);
            }
        }

    };

}