
#pragma once

#include <engine/arena.hpp>
#include "../toasts.hpp"
#include "../world/item.hpp"
#include <variant>
#include <unordered_map>

namespace houseofatmos::research {

    using namespace houseofatmos::engine::math;


    struct Research {

        struct ItemCondition {
            struct Progress {
                u64 produced_count = 0;
            };

            world::Item::Type item;
            u64 required_count;
        };

        enum Advancement {
            SteamEngines,
                RewardSteel,
                RewardPlanks,
                RewardOil,
                RewardBrassPots,
                RewardOilLanterns,
                RewardWatches,

            SteelBeams,
                RewardSteelBridges,
            
            PowerLooms,
                RewardFabric,
            
            BrassPots,
                RewardCheese,
                RewardSteak,
                RewardBeer
        };

        struct AdvancementInfo {
            std::string_view local_name;
            const ui::Background* icon;
            Vec<2> view_pos;

            std::span<const Advancement> parents;
            std::span<const ItemCondition> item_conditions;
        };

        static inline const f64 incr = 0.25;

        static inline const std::span<const AdvancementInfo> advancements 
                = (AdvancementInfo[]) {
            // Steam Engines and what it unlocks
            /* Advancement::SteamEngines */ {
                "item_name_steam_engines",
                &ui_icon::steam_engines,
                { 0.5 + 1 * incr, 0.5 + 0 * incr },
                {},
                (ItemCondition[]) { { world::Item::SteamEngines, 25 } }
            },
            /* Advancement::RewardSteel */ {
                "item_name_steel",
                &ui_icon::steel,
                { 0.5 + 2 * incr, 0.5 - 1 * incr },
                (Advancement[]) { Advancement::SteamEngines },
                {}
            },
            /* Advancement::RewardPlanks */ {
                "item_name_planks",
                &ui_icon::planks,
                { 0.5 + 2 * incr, 0.5 + 0 * incr },
                (Advancement[]) { Advancement::SteamEngines },
                {}
            },
            /* Advancement::RewardOil */ {
                "item_name_oil",
                &ui_icon::oil,
                { 0.5 + 2 * incr, 0.5 + 1 * incr },
                (Advancement[]) { Advancement::SteamEngines },
                {}
            },
            /* Advancement::RewardBrassPots */ {
                "item_name_brass_pots",
                &ui_icon::brass_pots,
                { 0.5 + 3 * incr, 0.5 - 1 * incr },
                (Advancement[]) { Advancement::SteamEngines },
                {}
            },
            /* Advancement::RewardOilLanterns */ {
                "item_name_oil_lanterns",
                &ui_icon::oil_lanterns,
                { 0.5 + 3 * incr, 0.5 + 0 * incr },
                (Advancement[]) { Advancement::SteamEngines },
                {}
            },
            /* Advancement::RewardWatches */ {
                "item_name_watches",
                &ui_icon::watches,
                { 0.5 + 3 * incr, 0.5 + 1 * incr },
                (Advancement[]) { Advancement::SteamEngines },
                {}
            },

            // Steel Beams and what it unlocks
            /* Advancement::SteelBeams */ {
                "item_name_steel_beams",
                &ui_icon::steel_beams,
                { 0.5 + 0 * incr, 0.5 + 1 * incr },
                (Advancement[]) { Advancement::SteamEngines },
                (ItemCondition[]) { { world::Item::SteelBeams, 200 } }
            },
            /* Advancement::RewardSteelBridges */ {
                "bridge_name_metal",
                &ui_icon::metal_bridge,
                { 0.5 + 0 * incr, 0.5 + 2 * incr },
                (Advancement[]) { Advancement::SteelBeams },
                {}
            },

            // Power Looms and what it unlocks
            /* Advancement::PowerLooms */ {
                "item_name_power_looms",
                &ui_icon::power_looms,
                { 0.5 - 1 * incr, 0.5 + 0 * incr },
                (Advancement[]) { Advancement::SteamEngines },
                (ItemCondition[]) { { world::Item::PowerLooms, 25 } }
            },
            /* Advancement::RewardFabric */ {
                "item_name_fabric",
                &ui_icon::fabric,
                { 0.5 - 2 * incr, 0.5 + 0 * incr },
                (Advancement[]) { Advancement::PowerLooms },
                {}
            },
            
            // Brass Pots and what it unlocks
            /* Advancement::BrassPots */ {
                "item_name_brass_pots",
                &ui_icon::brass_pots,
                { 0.5 + 0 * incr, 0.5 - 1 * incr },
                (Advancement[]) { Advancement::SteamEngines },
                (ItemCondition[]) { { world::Item::BrassPots, 50 } }
            },
            /* Advancement::RewardCheese */ {
                "item_name_cheese",
                &ui_icon::cheese,
                { 0.5 - 1 * incr, 0.5 - 2 * incr },
                (Advancement[]) { Advancement::BrassPots },
                {}
            },
            /* Advancement::RewardSteak */ {
                "item_name_steak",
                &ui_icon::steak,
                { 0.5 + 0 * incr, 0.5 - 2 * incr },
                (Advancement[]) { Advancement::BrassPots },
                {}
            },
            /* Advancement::RewardBeer */ {
                "item_name_beer",
                &ui_icon::beer,
                { 0.5 + 1 * incr, 0.5 - 2 * incr },
                (Advancement[]) { Advancement::BrassPots },
                {}
            }
        };



        struct AdvancementProgress {
            struct Serialized {
                bool is_unlocked;
                u64 item_conds_count, item_conds_offset;
            };

            bool is_unlocked = false;
            std::vector<ItemCondition::Progress> item_conditions;

            AdvancementProgress(size_t item_cond_count = 0);
            AdvancementProgress(
                const Serialized& serialized, const engine::Arena& buffer
            );
            Serialized serialize(engine::Arena& buffer) const;
        };



        struct Serialized {
            u64 items_count, items_offset;
        };

        std::unordered_map<Advancement, AdvancementProgress> progress;

        Research();
        Research(const Serialized& serialized, const engine::Arena& buffer);
        Serialized serialize(engine::Arena& buffer) const;

        void report_item_production(world::Item::Type item, u64 count);
        void check_completion(Toasts& toasts);
        
        bool is_unlocked(Advancement advancement) const {
            return this->progress.at(advancement).is_unlocked;
        }
    
        bool parents_unlocked(Advancement child) const {
            std::span<const Advancement> parents 
                = Research::advancements[(size_t) child].parents;
            for(Advancement parent: parents) {
                if(!this->progress.at(parent).is_unlocked) { 
                    return false; 
                }
            }
            return true;
        }

    };

}