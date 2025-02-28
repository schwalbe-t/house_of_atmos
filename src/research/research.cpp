
#include "research.hpp"

namespace houseofatmos::research {

    Research::AdvancementProgress::AdvancementProgress(size_t item_cond_count) {
        this->item_conditions.resize(item_cond_count);
    }

    Research::AdvancementProgress::AdvancementProgress(
        const Serialized& serialized, const engine::Arena& buffer
    ) {
        this->is_unlocked = serialized.is_unlocked;
        buffer.copy_array_at_into(
            serialized.item_conds_offset, serialized.item_conds_count, 
            this->item_conditions
        );
    }

    Research::AdvancementProgress::Serialized 
    Research::AdvancementProgress::serialize(engine::Arena& buffer) const {
        return {
            this->is_unlocked,
            this->item_conditions.size(), 
            buffer.alloc_array(this->item_conditions)
        };
    }



    Research::Research() {
        this->progress.reserve(Research::advancements.size());
        for(size_t adv_i = 0; adv_i < this->progress.size(); adv_i += 1) {
            this->progress[(Research::Advancement) adv_i] = AdvancementProgress(
                Research::advancements[adv_i].item_conditions.size()
            );
        }
    }

    Research::Research(const Serialized& serialized, const engine::Arena& buffer) {
        std::unordered_map<Advancement, AdvancementProgress::Serialized> items;
        buffer.copy_map_at_into(
            serialized.items_offset, serialized.items_count, items
        );
        this->progress.reserve(items.size());
        for(size_t adv_i = 0; adv_i < this->progress.size(); adv_i += 1) {
            auto advancement = (Research::Advancement) adv_i;
            this->progress[advancement] = items.contains(advancement)
                ? AdvancementProgress(items[advancement], buffer)
                : AdvancementProgress(
                    Research::advancements[adv_i].item_conditions.size()
                );
        }
    }

    Research::Serialized Research::serialize(engine::Arena& buffer) const {
        std::unordered_map<Advancement, AdvancementProgress::Serialized> items;
        items.reserve(this->progress.size());
        for(const auto& [advancement, progress]: this->progress) {
            items[advancement] = progress.serialize(buffer);
        }
        return {
            items.size(), buffer.alloc_map(items)
        };
    }


    void Research::report_item_production(world::Item::Type item, u64 count) {
        for(auto& [advancement, progress]: this->progress) {
            const AdvancementInfo& info 
                = Research::advancements[(size_t) advancement];
            size_t item_c_c = progress.item_conditions.size();
            for(size_t item_c_i = 0; item_c_i < item_c_c; item_c_i += 1) {
                if(info.item_conditions[item_c_i].item != item) { continue; }
                progress.item_conditions[item_c_i].produced_count += count;
            }
        }
    }

    void Research::check_completion(Toasts& toasts) {
        for(auto& [advancement, progress]: this->progress) {
            if(progress.is_unlocked) { continue; }
            const AdvancementInfo& info 
                = Research::advancements[(size_t) advancement];
            progress.is_unlocked = this->parents_unlocked(advancement);
            size_t item_c_c = progress.item_conditions.size();
            for(size_t item_c_i = 0; item_c_i < item_c_c; item_c_i += 1) {
                u64 required = info.item_conditions[item_c_i].required_count;
                u64 produced = progress.item_conditions[item_c_i].produced_count;
                progress.is_unlocked &= (produced >= required);
            }
            if(!progress.is_unlocked) { continue; }
            const std::string& name = toasts.localization().text(info.local_name);
            toasts.add_toast("toast_research_complete", { name });
        }
    }

}