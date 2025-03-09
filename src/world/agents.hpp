
#pragma once

#include "../renderer.hpp"

namespace houseofatmos::world {

    using namespace houseofatmos::engine::math;


    template<typename NodeIdentifier>
    struct AgentNetwork {
        using NodeId = NodeIdentifier;

        virtual void collect_node_neighbours(
            NodeId node, std::vector<NodeId> out
        ) const = 0;

        virtual void collect_node_positions(
            std::optional<NodeId> prev, NodeId node, std::optional<NodeId> next,
            std::vector<Vec<3>> out
        ) const = 0;

        virtual ~AgentNetwork() = default;

    };


    template<typename Network>
    struct AgentPath {

        struct Section {
            Network::NodeId node;
            std::vector<Vec<3>> points; 

            f64 length() const {
                f64 total = 0.0;
                for(size_t p = 1; p < this->points.size(); p += 1) {
                    total += (this->points[p] - this->points[p - 1]).len();
                }
                return total;
            }
        };

        std::vector<Section> sections;

        bool is_empty() const { return this->sections.size() == 0; }

        std::pair<size_t, f64> section_after(f64 distance) const {
            if(this->sections.size() == 0) {
                engine::error("Agent path is empty!");
            }
            size_t section_i = 0;
            f64 remaining = distance;
            for(;;) {
                if(section_i >= this->sections.size()) { break; }
                f64 len = this->sections[section_i].length();
                if(len > remaining) { break; }
                remaining -= len;
            }
            return std::pair<size_t, f64>(section_i, remaining);
        }

        Vec<3> position_after(f64 distance) const {
            auto [section_i, remaining] = this->section_after(distance);
            Section& section = this->sections[section_i];
            // Note: 'section_after' will never return a length 0 section
            Vec<3> position = section.points[0];
            for(size_t p = 1; p < section.points.size(); p += 1) {
                Vec<3> point = section.points[p];
                Vec<3> step = point - position;
                f64 step_len = step.len();
                f64 step_progress = std::min(remaining / step_len, 1.0);
                position += step * step_progress;
                if(remaining <= step_len) { break; }
                remaining -= step_len;
            }
            return position;
        }

    };


    template<typename Network>
    struct Agent {

        enum State {
            Travelling, Loading, Lost
        };

        State state;
        AgentPath<Network> path;
        f64 distance = 0.0;

        virtual f64 determine_speed() const = 0;

        // more stuff for actually travelling, doing item transfers,
        // handling state

        void render(
            Renderer& renderer, engine::Scene& scene, 
            const engine::Window& window
        ) {
            (void) renderer;
            (void) scene;
            (void) window;
        }

        virtual ~Agent() = default;
        
    };


    template<typename Agent, typename Network>
    struct AgentManager {
        
        std::vector<Agent> agents;

        // more stuff for updating all agents, 

    };



    struct CarriageNetwork: AgentNetwork<std::pair<u64, u64>> {

        void collect_node_neighbours(
            NodeId node, std::vector<NodeId> out
        ) const override {
            // todo
        }

        void collect_node_positions(
            std::optional<NodeId> prev, NodeId node, std::optional<NodeId> next,
            std::vector<Vec<3>> out
        ) const override {
            // todo
        }

    };

    struct Carriage: Agent<CarriageNetwork> {
        
        f64 determine_speed() const override {
            // todo
        }

    };

    using CarriageManager = AgentManager<CarriageNetwork, Carriage>;

}