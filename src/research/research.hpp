
#pragma once

#include <engine/arena.hpp>
#include "../toasts.hpp"
#include "../world/item.hpp"
#include <variant>
#include <unordered_map>

namespace houseofatmos::research {

    using namespace houseofatmos::engine::math;


    struct Research {

        enum struct Condition {
            SteamEngines,
            BrassPots,
            PowerLooms,
            SteelBeams,
            CoalLocomotives
        };

        struct ConditionInfo {
            struct Progress {
                u64 produced;

                Progress(): produced(0) {}
            };

            std::string_view local_name;
            std::vector<Condition> parents;
            world::Item::Type item;
            u64 required;
        };

        static const std::vector<ConditionInfo>& conditions();

        enum struct Reward {
            Steel, SteelBeams, BrassPots, Oil, OilLanterns, Watches, PowerLooms,
            Steak, Cheese, Beer,
            Fabric, Clothing,
            Tracking, LocomotiveFrames, SteelBridges,
            BasicLocomotive, SmallLocomotive, Tram
        };

        struct RewardInfo {
            std::string_view local_name;
            std::vector<Condition> required;  
        };

        static const std::vector<RewardInfo>& rewards();




        struct Serialized {
            engine::Arena::Map<Condition, ConditionInfo::Progress> progress;
        };

        std::unordered_map<Condition, ConditionInfo::Progress> progress;

        Research();
        Research(const Serialized& serialized, const engine::Arena& buffer);
        Serialized serialize(engine::Arena& buffer) const;

        void report_item_production(
            world::Item::Type item, u64 count, Toasts& toasts
        );
        
        bool is_unlocked(Condition condition) const {
            const ConditionInfo& cond_info = Research::conditions()
                .at((size_t) condition);
            for(const Condition& parent: cond_info.parents) {
                if(!this->is_unlocked(parent)) { return false; }
            }
            return this->progress.at(condition).produced 
                >= cond_info.required;
        }

        bool is_unlocked(Reward reward) const {
            const RewardInfo& reward_info = Research::rewards()
                .at((size_t) reward);
            for(const Condition& cond: reward_info.required) {
                if(!this->is_unlocked(cond)) { return false; }
            }
            return true;
        }

    };

}