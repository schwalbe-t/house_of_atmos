
#include "research.hpp"

namespace houseofatmos::research {

    static std::vector<Research::ConditionInfo> condition_infos = {
        /* SteamEngines */ {
            "item_name_steam_engines",
            {},
            world::Item::SteamEngines, 25
        },
        /* BrassPots */ {
            "item_name_brass_pots",
            { Research::Condition::SteamEngines },
            world::Item::BrassPots, 50
        },
        /* PowerLooms */ {
            "item_name_power_looms",
            { Research::Condition::SteamEngines },
            world::Item::PowerLooms, 25
        },
        /* SteelBeams */ {
            "item_name_steel_beams",
            { Research::Condition::SteamEngines },
            world::Item::SteelBeams, 50
        },
        /* CoalLocomotives */ {
            "item_name_locomotive_frames",
            { Research::Condition::SteelBeams },
            world::Item::LocomotiveFrames, 25
        }
    };

    const std::vector<Research::ConditionInfo>& Research::conditions() {
        return condition_infos;
    }


    static std::vector<Research::RewardInfo> reward_infos = {
        /* Steel */ { 
            "item_name_steel", { Research::Condition::SteamEngines } 
        },
        /* SteelBeams */ { 
            "item_name_steel_beams", { Research::Condition::SteamEngines } 
        },
        /* BrassPots */ { 
            "item_name_brass_pots", { Research::Condition::SteamEngines } 
        },
        /* Oil */ { 
            "item_name_oil", { Research::Condition::SteamEngines } 
        },
        /* OilLanterns */ { 
            "item_name_oil_lanterns", { Research::Condition::SteamEngines } 
        },
        /* Watches */ { 
            "item_name_watches", { Research::Condition::SteamEngines } 
        },
        /* PowerLooms */ { 
            "item_name_power_looms", { Research::Condition::SteamEngines } 
        },

        /* Steak */ { 
            "item_name_steak", { Research::Condition::BrassPots } 
        },
        /* Cheese */ { 
            "item_name_cheese", { Research::Condition::BrassPots } 
        },
        /* Beer */ { 
            "item_name_beer", { Research::Condition::BrassPots } 
        },

        /* Fabric */ { 
            "item_name_fabric", { Research::Condition::PowerLooms } 
        },
        /* Clothing */ { 
            "item_name_clothing", { Research::Condition::PowerLooms } 
        },

        /* Tracking */ { 
            "ui_train_tracks", { Research::Condition::SteelBeams }
        },
        /* LocomotiveFrames */ {
            "item_name_locomotive_frames", { Research::Condition::SteelBeams }
        },
        /* SteelBridges */ { 
            "bridge_name_metal", { Research::Condition::SteelBeams } 
        },

        /* BasicLocomotive */ {
            "locomotive_name_basic", { Research::Condition::CoalLocomotives } 
        },
        /* SmallLocomotive */ { 
            "locomotive_name_small", { Research::Condition::CoalLocomotives } 
        },
        /* Tram */ { 
            "locomotive_name_tram", { Research::Condition::CoalLocomotives } 
        }
    };

    const std::vector<Research::RewardInfo>& Research::rewards() {
        return reward_infos;
    }



    Research::Research() {
        this->progress.reserve(Research::conditions().size());
        for(size_t c = 0; c < Research::conditions().size(); c += 1) {
            this->progress[(Condition) c] = {};
        }
    }

    Research::Research(const Serialized& serialized, const engine::Arena& buffer) {
        buffer.copy_into(serialized.progress, this->progress);
        for(size_t c = 0; c < Research::conditions().size(); c += 1) {
            if(this->progress.contains((Condition) c)) { continue; }
            this->progress[(Condition) c] = {};
        }
    }

    Research::Serialized Research::serialize(engine::Arena& buffer) const {
        return {
            buffer.alloc(this->progress)
        };
    }


    void Research::report_item_production(
        world::Item::Type item, u64 count, Toasts& toasts
    ) {
        for(auto& [condition, progress]: this->progress) {
            const ConditionInfo& info 
                = Research::conditions().at((size_t) condition);
            if(info.item != item) { continue; }
            bool was_completed = progress.produced >= info.required;
            progress.produced += count;
            bool is_completed = progress.produced >= info.required;
            if(is_completed) { progress.produced = info.required; }
            if(is_completed && !was_completed) {
                const std::string& name = toasts.localization()
                    .text(info.local_name);
                toasts.add_toast("toast_research_complete", { name });
            }
        }
    }

}