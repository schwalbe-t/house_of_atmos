
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
            Metalworking,
            TestBeer, TestBread
        };

        struct AdvancementInfo {
            std::string local_name;
            const ui::Background* icon;
            Vec<2> view_pos;

            std::vector<Advancement> parents;
            std::vector<ItemCondition> item_conditions;
        };

        static inline const f64 incr = 0.375;

        static inline const std::unordered_map<Advancement, AdvancementInfo> 
            advancements = {
            { Advancement::Metalworking, {
                "research_name_metalworking",
                &ui_icon::steel,
                { 0.5, 0.5 },
                {},
                { { world::Item::Steel, 25 } }
            } },
            { Advancement::TestBeer, {
                "item_name_beer",
                &ui_icon::beer,
                { 0.5 + incr, 0.5 },
                { Advancement::Metalworking },
                { { world::Item::Beer, 1000 } }
            } },
            { Advancement::TestBread, {
                "item_name_bread",
                &ui_icon::bread,
                { 0.5, 0.5 + incr },
                { Advancement::Metalworking },
                { { world::Item::Bread, 1000 } }
            } }
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
            for(Advancement parent: Research::advancements.at(child).parents) {
                if(!this->progress.at(parent).is_unlocked) { return false; }
            }
            return true;
        }

    };

}