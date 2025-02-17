
#pragma once

#include <engine/arena.hpp>
#include "item.hpp"
#include "../toasts.hpp"
#include <variant>
#include <unordered_map>

namespace houseofatmos::world {

    struct Research {

        struct ItemCondition {
            struct Progress {
                u64 produced_count = 0;
            };

            Item::Type item;
            u64 required_count;
        };

        enum Advancement {
            Metalworking
        };

        struct AdvancementInfo {
            std::string local_name;
            std::vector<Advancement> parents;
            std::vector<ItemCondition> item_conditions;
        };

        static inline const std::unordered_map<Advancement, AdvancementInfo> 
            advancements = {
            { Advancement::Metalworking, {
                "research_name_metalworking",
                {},
                { { Item::Steel, 25 } }
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

        private:
        std::unordered_map<Advancement, AdvancementProgress> progress;

        public:
        Research();
        Research(const Serialized& serialized, const engine::Arena& buffer);
        Serialized serialize(engine::Arena& buffer) const;

        void report_item_production(Item::Type item, u64 count);
        void check_completion(Toasts& toasts);
        bool is_unlocked(Advancement advancement) const;

    };

}