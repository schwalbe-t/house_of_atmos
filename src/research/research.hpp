
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

            std::vector<Advancement> parents;
            std::vector<ItemCondition> item_conditions;
        };

        static const std::vector<AdvancementInfo>& advancements();



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
                = Research::advancements().at((size_t) child).parents;
            for(Advancement parent: parents) {
                if(!this->progress.at(parent).is_unlocked) { 
                    return false; 
                }
            }
            return true;
        }

    };

}