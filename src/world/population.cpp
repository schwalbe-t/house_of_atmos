
#include "terrain.hpp"
#include "complex.hpp"
#include "population.hpp"

namespace houseofatmos::world {

    static inline f64 min_population = 25.0;

    Population::Population(u64 tile_x, u64 tile_z, PopulationName name):
        tile({ tile_x, tile_z }), name(name),
        size(min_population), resources(0.0) {}

    Population::Population(
        const Serialized& serialized, const engine::Arena& buffer
    ) {
        this->tile = serialized.tile;
        this->name = serialized.name;
        this->size = serialized.size;
        this->resources = serialized.resources;
        buffer.copy_into(serialized.houses, this->houses);
        buffer.copy_into(serialized.markets, this->markets);
    }

    Population::Serialized Population::serialize(engine::Arena& buffer) const {
        return {
            this->tile,
            this->name,
            this->size,
            this->resources,
            buffer.alloc(this->houses),
            buffer.alloc(this->markets)
        };
    }

    struct SettlementMarketInfo {
        f64 min_population;
        Complex::Member c_member;
    };

    static std::vector<SettlementMarketInfo> markets = {
        // need to be in ascending order
        { 0.0, Complex::Member({
            Conversion({ { 1, Item::Bread       } }, { {  10, Item::Coins } }, 0.05),
            Conversion({ { 1, Item::Milk        } }, { {  10, Item::Coins } }, 0.05)
        }) },
        { 200.0, Complex::Member({
            Conversion({ { 1, Item::BrassPots   } }, { { 100, Item::Coins } }, 0.50),
            Conversion({ { 1, Item::Watches     } }, { {  75, Item::Coins } }, 0.40),
            Conversion({ { 1, Item::OilLanterns } }, { {  30, Item::Coins } }, 0.10)
        }) },
        { 500.0, Complex::Member({
            Conversion({ { 1, Item::Clothing    } }, { {  50, Item::Coins } }, 0.20),
            Conversion({ { 1, Item::Tools       } }, { {  75, Item::Coins } }, 0.40),
            Conversion({ { 1, Item::Armor       } }, { { 100, Item::Coins } }, 0.50)
        }) },
        { 1000.0, Complex::Member({
            Conversion({ { 1, Item::Oil         } }, { {  30, Item::Coins } }, 0.10),
            Conversion({ { 1, Item::Cheese      } }, { {  30, Item::Coins } }, 0.10),
            Conversion({ { 1, Item::Steak       } }, { {  30, Item::Coins } }, 0.10),
            Conversion({ { 1, Item::Beer        } }, { {  30, Item::Coins } }, 0.10)
        }) }
    };    

    static f64 resource_value_of(u64 population, Item::Type item) {
        if(population >= markets[0].min_population) {
            switch(item) {
                case Item::Bread:       return 100.0;
                case Item::Milk:        return 100.0;
                default:;
            }
        }
        if(population >= markets[1].min_population) {
            switch(item) {
                case Item::BrassPots:   return 200.0;
                case Item::Watches:     return 200.0;
                case Item::OilLanterns: return 300.0;
                default:;
            }
        }
        if(population >= markets[2].min_population) {            
            switch(item) {            
                case Item::Clothing:    return 300.0;
                case Item::Tools:       return 300.0;
                case Item::Armor:       return 300.0;
                default:;
            }
        }
        if(population >= markets[3].min_population) {
            switch(item) {
                case Item::Oil:         return 100.0;
                case Item::Cheese:      return 300.0;
                case Item::Steak:       return 400.0;
                case Item::Beer:        return 500.0;
                default:;
            }
        }
        engine::warning("'resource_value_of': Unhandled item type " 
            + std::to_string(item) + "!"
        );
        return 0.0;
    }

    void Population::report_item_purchase(Item::Type item, u64 count) {
        this->resources += resource_value_of(this->size, item) * (f64) count;
    }

    static inline f64 growth_threshold_period = 60.0;
    static inline f64 growth_speed = 1.0;
    static inline f64 shrinking_threshold_period = 20.0;
    static inline f64 shrink_speed = 0.5;

    static void update_size(Population& p, const engine::Window& window) {
        f64 growth_threshold 
            = p.consumption_sum(growth_threshold_period);
        if(p.resources >= growth_threshold) {
            p.size += growth_speed * window.delta_time();
        }
        f64 shrinking_threshold 
            = p.consumption_sum(shrinking_threshold_period);
        if(p.resources < shrinking_threshold) {
            p.size -= shrink_speed * window.delta_time();
            if(p.size < min_population) { p.size = min_population; }
        }
        p.resources -= p.consumption_sum(window.delta_time());
        if(p.resources < 0.0) { p.resources = 0.0; }
    }

    static bool building_placement_allowed(
        const Terrain& terrain, u64 x, u64 z, Building::Type type
    ) {
        const Building::TypeInfo& type_info = Building::types()
            .at((size_t) type);
        return terrain.vert_area_above_water(
            x, z, x + type_info.width, z + type_info.height
        ) && terrain.vert_area_elev_mutable(
            x, z, x + type_info.width, z + type_info.height
        );
    }

    static inline f64 people_per_house = 10.0;

    static void update_houses(Population& p, Terrain& terrain) {
        auto new_houses_end = std::remove_if(
            p.houses.begin(), p.houses.end(),
            [&terrain](const auto& h) { 
                return terrain.building_at((i64) h.first, (i64) h.second) 
                    == nullptr; 
            }
        );
        p.houses.erase(new_houses_end, p.houses.end());
        u64 target_houses = (u64) (p.size / people_per_house);
        while(p.houses.size() > target_houses) {
            auto [rem_x, rem_z] = p.houses.back();
            u64 ch_x = rem_x / terrain.tiles_per_chunk();
            u64 ch_z = rem_z / terrain.tiles_per_chunk();
            Terrain::ChunkData& chunk = terrain.chunk_at(ch_x, ch_z);
            for(size_t b = 0; b < chunk.buildings.size(); b += 1) {
                const Building& building = chunk.buildings[b];
                u64 x = building.x + ch_x * terrain.tiles_per_chunk();
                u64 z = building.z + ch_z * terrain.tiles_per_chunk();
                if(x != rem_x || z != rem_z) { continue; }
                chunk.buildings.erase(chunk.buildings.begin() + b);
                break;
            }
            p.houses.pop_back();
        }
        for(size_t h = p.houses.size(); h < target_houses; h += 1) {
            for(size_t att_c = 0; att_c < 10; att_c += 1) {
                f64 angle = terrain.rng.next_f64() * 2.0 * pi;
                f64 dist = terrain.rng.next_f64() * p.radius();
                u64 x = (u64) (p.tile.first + (i64) (cos(angle) * dist));
                u64 z = (u64) (p.tile.second + (i64) (sin(angle) * dist));
                bool allowed = building_placement_allowed(
                    terrain, x, z, Building::House
                );
                if(!allowed) { continue; }
                terrain.place_building(Building::House, x, z);
                p.houses.push_back({ x, z });
                break;
            }
        }
    }

    static void update_markets(
        Population& p, Terrain& terrain, ComplexBank& complexes
    ) {
        for(u16 m = p.markets.size(); m < markets.size(); m += 1) {
            const SettlementMarketInfo& market = markets[m];
            if(p.size < market.min_population) { break; }
            bool was_placed = false;
            for(size_t att_c = 0; att_c < 10; att_c += 1) {
                f64 angle = terrain.rng.next_f64() * 2.0 * pi;
                f64 dist = terrain.rng.next_f64() * p.radius();
                u64 x = (u64) (p.tile.first + (i64) (cos(angle) * dist));
                u64 z = (u64) (p.tile.second + (i64) (sin(angle) * dist));
                bool allowed = building_placement_allowed(
                    terrain, x, z, Building::Plaza
                );
                if(!allowed) { continue; }
                ComplexId market_complex = complexes.create_complex();
                complexes.get(market_complex).add_member(x, z, market.c_member);
                terrain.place_building(Building::Plaza, x, z, market_complex);
                p.markets.push_back({ { x, z }, m });
                break;
            }
            if(!was_placed) { break; }
        }
    }

    void Population::update(
        const engine::Window& window, Terrain& terrain, ComplexBank& complexes
    ) {
        update_size(*this, window);
        update_houses(*this, terrain);
        update_markets(*this, terrain, complexes);
    }



    PopulationManager::PopulationManager(
        std::function<void (PopulationManager&)> stop_register_handler
    ): stop_register_handler(stop_register_handler) {}

    PopulationManager::PopulationManager(
        const Serialized& serialized, const engine::Arena& buffer,
        std::function<void (PopulationManager&)> stop_register_handler
    ) {
        buffer.copy_into<Population::Serialized, Population>(
            serialized.populations, this->populations, 
            [&buffer](const auto& p) { return Population(p, buffer); }
        );
        this->stop_register_handler = stop_register_handler;
    }

    PopulationManager::Serialized PopulationManager::serialize(
        engine::Arena& buffer
    ) const {
        return {
            buffer.alloc<Population, Population::Serialized>(
                this->populations,
                [&buffer](const auto& p) { return p.serialize(buffer); }
            )
        };
    }

    void PopulationManager::reset() {
        // TODO!
        // reset groups and nodes
        // create nodes and groups for all populations
        //     (scale node size with population size)
        // call the handler
        // combine the groups of all nearby nodes
    }

    void PopulationManager::report_item_purchase(
        Item::Type item, u64 count, ComplexId complex, 
        const Terrain& terrain
    ) {
        for(Population& p: this->populations) {
            bool is_target = false;
            for(const SettlementMarket& m: p.markets) {
                auto [tile_x, tile_z] = m.tile;
                const Building* b = terrain.building_at(tile_x, tile_z);
                is_target |= b != nullptr 
                    && b->complex.has_value()
                    && b->complex->index == complex.index;
                if(is_target) { break; }
            }
            if(!is_target) { continue; }
            p.report_item_purchase(item, count);
            break;
        }
    }

    void PopulationManager::update(
        const engine::Window& window, Terrain& terrain, ComplexBank& complexes
    ) {
        for(Population& population: this->populations) {
            population.update(window, terrain, complexes);
        }
    }

    std::optional<PopulationGroupId> PopulationManager::group_at(
        u64 tile_x, u64 tile_z
    ) const {
        // TODO!
        // find the closest node to the tile (return if there is none)
        // return its group
    }

}