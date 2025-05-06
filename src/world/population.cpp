
#include "terrain.hpp"
#include "complex.hpp"
#include "population.hpp"
#include "agent.hpp"

namespace houseofatmos::world {

    static inline f64 min_population = 50.0;

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
                f64 dist = terrain.rng.next_f64() * p.building_radius();
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
                f64 dist = terrain.rng.next_f64() * p.building_radius();
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

    void PopulationManager::register_stops(
        const std::vector<AgentStop>& schedule, f64 radius, 
        const ComplexBank& complexes
    ) {
        auto group = PopulationGroupId(this->groups.size());
        this->groups.push_back(PopulationGroup());
        for(const AgentStop& stop: schedule) {
            const Complex& target = complexes.get(stop.target);
            if(target.get_members().size() == 0) { continue; }
            u64 x = 0;
            u64 z = 0;
            for(const auto& member: target.get_members()) {
                x += member.first.first;
                z += member.first.second;
            }
            x /= target.get_members().size();
            z /= target.get_members().size();
            this->nodes.push_back({ group, { x, z }, radius });
        }
    }

    static void register_populations(PopulationManager& pm) {
        for(u32 p_id = 0; p_id < pm.populations.size(); p_id += 1) {
            const Population& p = pm.populations[p_id];
            auto group_id = PopulationGroupId(pm.groups.size());
            PopulationGroup group;
            group.populations.push_back(PopulationId(p_id));
            pm.groups.push_back(group);
            pm.nodes.push_back({ group_id, p.tile, p.worker_radius() });
        }
    }

    static void merge_group_into(
        PopulationManager& pm, 
        PopulationGroupId from, PopulationGroupId into
    ) {
        if(from.index == into.index) { return; }
        PopulationGroup& from_g = pm.groups.at(from.index);
        PopulationGroup& into_g = pm.groups.at(into.index);
        for(PopulationId pid: from_g.populations) {
            bool exists = false;
            for(PopulationId e_pid: into_g.populations) {
                exists |= e_pid.index == pid.index;
                if(exists) { break; }
            }
            if(!exists) {
                into_g.populations.push_back(pid);
            }
        }
        // force deallocation of the internal vector buffer
        // (we know the group won't be used anymore)
        std::vector<PopulationId>().swap(from_g.populations);
        for(PopulationNode& n: pm.nodes) {
            if(n.group.index == from.index) { n.group = into; }
        }
    }

    static void merge_nearby_nodes(PopulationManager& pm) {
        for(const PopulationNode& node_a: pm.nodes) {
            Vec<2> tile_a = Vec<2>(node_a.tile.first, node_a.tile.second);
            for(const PopulationNode& node_b: pm.nodes) {
                if(node_a.group.index == node_b.group.index) { continue; }
                Vec<2> tile_b = Vec<2>(node_b.tile.first, node_b.tile.second);
                f64 dist = (tile_a - tile_b).len();
                if(dist > node_a.radius + node_b.radius) { continue; }
                merge_group_into(pm, node_a.group, node_b.group);
            }
        }
    }

    static void update_worker_distribution(
        PopulationManager& pm, Terrain& terrain, Toasts* toasts
    ) {
        for(PopulationGroup& g: pm.groups) {
            g.available = 0.0;
            for(PopulationId p_id: g.populations) {
                g.available += pm.populations[p_id.index].size;
            }
        }
        bool all_working = true;
        for(u64 ch_x = 0; ch_x < terrain.width_in_chunks(); ch_x += 1) {
            for(u64 ch_z = 0; ch_z < terrain.height_in_chunks(); ch_z += 1) {
                Terrain::ChunkData& chunk = terrain.chunk_at(ch_x, ch_z);
                for(Building& building: chunk.buildings) {
                    const Building::TypeInfo& building_info
                        = Building::types().at((size_t) building.type);
                    if(building_info.workers == 0) {
                        building.workers = Building::WorkerState::Working;
                        continue;
                    }
                    u64 t_x = ch_x * terrain.tiles_per_chunk() + building.x;
                    u64 t_z = ch_z * terrain.tiles_per_chunk() + building.z;
                    std::optional<PopulationGroupId> group_id
                        = pm.group_at(t_x, t_z);
                    if(!group_id.has_value()) {
                        building.workers = Building::WorkerState::Unreachable;
                        all_working = false;
                        continue;
                    }
                    PopulationGroup& group = pm.groups[group_id->index];
                    if(group.populations.size() == 0) {
                        building.workers = Building::WorkerState::Unreachable;
                        all_working = false;
                        continue;
                    }
                    f64 required = (f64) building_info.workers;
                    if(group.available < required) {
                        building.workers = Building::WorkerState::Shortage;
                        continue;
                    }
                    group.available -= required;
                    building.workers = Building::WorkerState::Working;
                }
            }
        }
        if(!all_working && toasts != nullptr) {
            toasts->add_error("toast_missing_workers", {});
        }
    }

    void PopulationManager::reset(Terrain& terrain, Toasts* toasts) {
        this->groups.clear();
        this->nodes.clear();
        register_populations(*this);
        this->stop_register_handler(*this);
        merge_nearby_nodes(*this);
        update_worker_distribution(*this, terrain, toasts);
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
        u64 closest_dist = UINT64_MAX;
        const PopulationNode* node = nullptr;
        for(const PopulationNode& n: this->nodes) {
            u64 dx = (u64) std::abs((i64) n.tile.first - (i64) tile_x);
            u64 dz = (u64) std::abs((i64) n.tile.second - (i64) tile_z);
            u64 dist = dx + dz;
            if(dist >= closest_dist) { continue; }
            node = &n;
            closest_dist = dist;
        }
        if(node == nullptr) { return std::nullopt; }
        Vec<2> node_pos = Vec<2>(node->tile.first, node->tile.second);
        f64 true_dist = (Vec<2>(tile_x, tile_z) - node_pos).len();
        if(true_dist > node->radius) { return std::nullopt; }
        return node->group;
    }

}