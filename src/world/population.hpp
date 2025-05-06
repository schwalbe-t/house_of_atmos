
#pragma once

#include <engine/arena.hpp>
#include <engine/rng.hpp>
#include <engine/localization.hpp>
#include "item.hpp"
#include "complex_id.hpp"
#include <optional>

namespace houseofatmos::world {

    using namespace houseofatmos::engine::math;


    struct Terrain;
    struct ComplexBank;
    struct AgentStop;


    struct PopulationName {

        static inline const u32 prefix_count = 10;
        static inline const u32 suffix_count = 10;

        u32 prefix, suffix;

        PopulationName(): prefix(0), suffix(0) {}

        PopulationName(StatefulRNG& rng) {
            this->prefix = (u32) (rng.next_f64() * (f64) prefix_count);
            this->suffix = (u32) (rng.next_f64() * (f64) suffix_count);
        }

        bool operator==(const PopulationName& other) const { 
            return this->prefix == other.prefix
                && this->suffix == other.suffix;
        }

        std::string display(const engine::Localization& local) const {
            std::string prefix_key = "settlement_name_prefix_" 
                + std::to_string(this->prefix);
            std::string suffix_key = "settlement_name_suffix_"
                + std::to_string(this->suffix);
            return local.text(prefix_key) + local.text(suffix_key);
        }

    };


    struct SettlementMarket {
        std::pair<u64, u64> tile;
        u16 type;
    };

    struct PopulationId { u32 index; };
    struct Population {

        struct Serialized {
            std::pair<u64, u64> tile;
            PopulationName name;
            f64 size;
            f64 resources;
            engine::Arena::Array<std::pair<u64, u64>> houses;
            engine::Arena::Array<SettlementMarket> markets;
        };

        std::pair<u64, u64> tile;
        PopulationName name;
        f64 size;
        f64 resources;
        std::vector<std::pair<u64, u64>> houses;
        std::vector<SettlementMarket> markets;

        Population(u64 tile_x, u64 tile_z, PopulationName name);
        Population(const Serialized& serialized, const engine::Arena& buffer);

        Serialized serialize(engine::Arena& buffer) const;

        // 1 resource / person / second
        f64 consumption_sum(f64 seconds) const {
            return this->size * seconds;
        } 

        // 1 tile per 10.0 people
        f64 radius() const {
            return this->size / 10.0;
        }

        void report_item_purchase(Item::Type item, u64 count);

        void update(
            const engine::Window& window, 
            Terrain& terrain, ComplexBank& complexes
        );

    };

    struct PopulationGroupId { u32 index; };
    struct PopulationGroup {
        std::vector<PopulationId> populations;
    };

    struct PopulationNode {
        PopulationGroupId group;
        std::pair<u64, u64> tile;
        f64 radius;
    };


    struct PopulationManager {

        struct Serialized {
            engine::Arena::Array<Population::Serialized> populations;
        };

        std::vector<Population> populations;

        std::function<void (PopulationManager&)> stop_register_handler;
        std::vector<PopulationGroup> groups;
        std::vector<PopulationNode> nodes;

        PopulationManager(
            std::function<void (PopulationManager&)> stop_register_handler
        );
        PopulationManager(
            const Serialized& serialized, const engine::Arena& buffer,
            std::function<void (PopulationManager&)> stop_register_handler
        );

        Serialized serialize(engine::Arena& buffer) const;

        void register_stops(
            const std::vector<AgentStop>& schedule, f64 radius, 
            const ComplexBank& complexes
        );

        void reset();

        void report_item_purchase(
            Item::Type item, u64 count, ComplexId complex, 
            const Terrain& terrain
        );

        void update(
            const engine::Window& window, 
            Terrain& terrain, ComplexBank& complexes
        );

        std::optional<PopulationGroupId> group_at(u64 tile_x, u64 tile_z) const;
        
    };

}