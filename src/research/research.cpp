
#include "research.hpp"

namespace houseofatmos::research {

    static const f64 incr = 0.25;

    static std::vector<Research::AdvancementInfo> advancement_infos = {
        // Steam Engines and what it unlocks
        /* Advancement::SteamEngines */ {
            "item_name_steam_engines",
            &ui_icon::steam_engines,
            { 0.5 + 1 * incr, 0.5 + 0 * incr },
            {},
            { { world::Item::SteamEngines, 25 } }
        },
        /* Advancement::RewardSteel */ {
            "item_name_steel",
            &ui_icon::steel,
            { 0.5 + 2 * incr, 0.5 - 1 * incr },
            { Research::Advancement::SteamEngines },
            {}
        },
        /* Advancement::RewardPlanks */ {
            "item_name_planks",
            &ui_icon::planks,
            { 0.5 + 2 * incr, 0.5 + 0 * incr },
            { Research::Advancement::SteamEngines },
            {}
        },
        /* Advancement::RewardOil */ {
            "item_name_oil",
            &ui_icon::oil,
            { 0.5 + 2 * incr, 0.5 + 1 * incr },
            { Research::Advancement::SteamEngines },
            {}
        },
        /* Advancement::RewardBrassPots */ {
            "item_name_brass_pots",
            &ui_icon::brass_pots,
            { 0.5 + 3 * incr, 0.5 - 1 * incr },
            { Research::Advancement::SteamEngines },
            {}
        },
        /* Advancement::RewardOilLanterns */ {
            "item_name_oil_lanterns",
            &ui_icon::oil_lanterns,
            { 0.5 + 3 * incr, 0.5 + 0 * incr },
            { Research::Advancement::SteamEngines },
            {}
        },
        /* Advancement::RewardWatches */ {
            "item_name_watches",
            &ui_icon::watches,
            { 0.5 + 3 * incr, 0.5 + 1 * incr },
            { Research::Advancement::SteamEngines },
            {}
        },

        // Steel Beams and what it unlocks
        /* Advancement::SteelBeams */ {
            "item_name_steel_beams",
            &ui_icon::steel_beams,
            { 0.5 + 0 * incr, 0.5 + 1 * incr },
            { Research::Advancement::SteamEngines },
            { { world::Item::SteelBeams, 200 } }
        },
        /* Advancement::RewardSteelBridges */ {
            "bridge_name_metal",
            &ui_icon::metal_bridge,
            { 0.5 + 0 * incr, 0.5 + 2 * incr },
            { Research::Advancement::SteelBeams },
            {}
        },

        // Power Looms and what it unlocks
        /* Advancement::PowerLooms */ {
            "item_name_power_looms",
            &ui_icon::power_looms,
            { 0.5 - 1 * incr, 0.5 + 0 * incr },
            { Research::Advancement::SteamEngines },
            { { world::Item::PowerLooms, 25 } }
        },
        /* Advancement::RewardFabric */ {
            "item_name_fabric",
            &ui_icon::fabric,
            { 0.5 - 2 * incr, 0.5 + 0 * incr },
            { Research::Advancement::PowerLooms },
            {}
        },
        
        // Brass Pots and what it unlocks
        /* Advancement::BrassPots */ {
            "item_name_brass_pots",
            &ui_icon::brass_pots,
            { 0.5 + 0 * incr, 0.5 - 1 * incr },
            { Research::Advancement::SteamEngines },
            { { world::Item::BrassPots, 50 } }
        },
        /* Advancement::RewardCheese */ {
            "item_name_cheese",
            &ui_icon::cheese,
            { 0.5 - 1 * incr, 0.5 - 2 * incr },
            { Research::Advancement::BrassPots },
            {}
        },
        /* Advancement::RewardSteak */ {
            "item_name_steak",
            &ui_icon::steak,
            { 0.5 + 0 * incr, 0.5 - 2 * incr },
            { Research::Advancement::BrassPots },
            {}
        },
        /* Advancement::RewardBeer */ {
            "item_name_beer",
            &ui_icon::beer,
            { 0.5 + 1 * incr, 0.5 - 2 * incr },
            { Research::Advancement::BrassPots },
            {}
        }
    };

    const std::vector<Research::AdvancementInfo>& Research::advancements() {
        return advancement_infos;
    }



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
        this->progress.reserve(Research::advancements().size());
        for(size_t adv_i = 0; adv_i < Research::advancements().size(); adv_i += 1) {
            this->progress[(Research::Advancement) adv_i] = AdvancementProgress(
                Research::advancements().at(adv_i).item_conditions.size()
            );
        }
    }

    Research::Research(const Serialized& serialized, const engine::Arena& buffer) {
        std::unordered_map<Advancement, AdvancementProgress::Serialized> items;
        buffer.copy_map_at_into(
            serialized.items_offset, serialized.items_count, items
        );
        this->progress.reserve(items.size());
        for(size_t adv_i = 0; adv_i < Research::advancements().size(); adv_i += 1) {
            auto advancement = (Research::Advancement) adv_i;
            this->progress[advancement] = items.contains(advancement)
                ? AdvancementProgress(items[advancement], buffer)
                : AdvancementProgress(
                    Research::advancements().at(adv_i).item_conditions.size()
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
                = Research::advancements().at((size_t) advancement);
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
                = Research::advancements().at((size_t) advancement);
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