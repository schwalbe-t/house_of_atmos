
#pragma once

#include <engine/arena.hpp>
#include <optional>

namespace houseofatmos::world {

    struct PopulationNodeId { u64 index; };
    struct PopulationGroupId { u32 index; };
    struct PopulationId { u32 index; };

    struct PopulationNode {
        std::vector<PopulationNodeId> connected;
        f64 radius;
        PopulationGroupId group;
    };

    struct PopulationGroup {
        std::vector<PopulationId> populations;
    };

    struct Population {
        PopulationNodeId node;
    };


    struct PopulationManager {

        struct Serialized {
            u64 populations_count, populations_offset;
        };

        std::vector<Population> populations;
        std::vector<PopulationNode> nodes;
        std::vector<PopulationGroup> groups;

        PopulationManager();
        PopulationManager(
            const Serialized& serialized, const engine::Arena& buffer
        );

        Serialized serialized(engine::Arena& buffer) const;

    };

}