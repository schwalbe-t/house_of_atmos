
#include "bridge.hpp"

namespace houseofatmos::world {

    static std::vector<Bridge::TypeInfo> type_infos = {
        /* Type::Wooden */ {
            "bridge_name_wooden",
            &ui_icon::wooden_bridge,
            { 
                "res/bridges/wooden_bridge.glb", Renderer::model_attribs,
                engine::FaceCulling::Enabled
            },
            {
                RelCollider({ -2.5, -3.0, -2.5 }, { 5.0, 2.5, 5.0 }),
                RelCollider({ -2.5 - 0.125, 0, -2.5 - 0.125 }, { 5.0 + 0.25, 1, 0.25 }),
                RelCollider({ -2.5 - 0.125, 0,  2.5 - 0.125 }, { 5.0 + 0.25, 1, 0.25 }),

                RelCollider({ -2.5 - 0.125, -12,  -2.5 - 0.125 }, { 0.25, 12, 0.25 }),
                RelCollider({ -2.5 - 0.125, -12,   2.5 - 0.125 }, { 0.25, 12, 0.25 }),
                RelCollider({  2.5 - 0.125, -12,  -2.5 - 0.125 }, { 0.25, 12, 0.25 }),
                RelCollider({  2.5 - 0.125, -12,   2.5 - 0.125 }, { 0.25, 12, 0.25 })
            },
            {
                RelCollider({ -2.5, -3.0, -2.5 }, { 5.0, 2.5, 5.0 }),
                RelCollider({ -2.5 - 0.125, 0, -2.5 - 0.125 }, { 0.25, 1, 5.0 + 0.25 }),
                RelCollider({  2.5 - 0.125, 0, -2.5 - 0.125 }, { 0.25, 1, 5.0 + 0.25 }),

                RelCollider({ -2.5 - 0.125, 0,  -2.5 - 0.125 }, { 0.25, 1, 0.25 }),
                RelCollider({ -2.5 - 0.125, 0,   2.5 - 0.125 }, { 0.25, 1, 0.25 }),
                RelCollider({  2.5 - 0.125, 0,  -2.5 - 0.125 }, { 0.25, 1, 0.25 }),
                RelCollider({  2.5 - 0.125, 0,   2.5 - 0.125 }, { 0.25, 1, 0.25 })
            },
            // min and max height (distance of terrain / water to bridge floor)
            3, 12, 
            false, // does not allow boats to pass under
            1000 // cost of bridge per tile
        },
        /* Type::Stone */ {
            "bridge_name_stone",
            &ui_icon::stone_bridge,
            { 
                "res/bridges/stone_bridge.glb", Renderer::model_attribs, 
                engine::FaceCulling::Enabled 
            },
            {
                RelCollider({ -2.5, -1.0, -2.5 }, { 5.0, 0.5, 5.0 }),
                RelCollider({ -2.5, 0, -2.5 }, { 5.0, 1, 0.5 }),
                RelCollider({ -2.5, 0,  2.0 }, { 5.0, 1, 0.5 }),

                RelCollider({ -2.5, -18, -2.5 }, { 0.5, 17, 5.0 }),
                RelCollider({  2.0, -18, -2.5 }, { 0.5, 17, 5.0 })
            },
            {
                RelCollider({ -2.5, -1.0, -2.5 }, { 5.0, 0.5, 5.0 }),
                RelCollider({ -2.5, 0, -2.5 }, { 0.5, 1, 5.0 }),
                RelCollider({  2.0, 0, -2.5 }, { 0.5, 1, 5.0 }),

                RelCollider({ -2.5, -18, -2.5 }, { 5.0, 17, 0.5 }),
                RelCollider({ -2.5, -18,  2.0 }, { 5.0, 17, 0.5 })
            },
            // min and max height (distance of terrain / water to bridge floor)
            2, 18, 
            false, // does not allow boats to pass under
            2000 // cost of bridge per tile
        },
        /* Type::Metal */ {
            "bridge_name_metal",
            &ui_icon::metal_bridge,
            { 
                "res/bridges/metal_bridge.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            },
            {
                RelCollider({ -2.5, -1.0, -2.5 }, { 5.0, 0.5, 5.0 }),
                RelCollider({ -2.5 - 0.25, 0, -2.5 - 0.25 }, { 5.0 + 0.5, 1, 0.5 }),
                RelCollider({ -2.5 - 0.25, 0,  2.5 - 0.25 }, { 5.0 + 0.5, 1, 0.5 }),
            },
            {
                RelCollider({ -2.5, -1.0, -2.5 }, { 5.0, 0.5, 5.0 }),
                RelCollider({ -2.5 - 0.25, 0, -2.5 - 0.25 }, { 0.5, 1, 5.0 + 0.5 }),
                RelCollider({  2.5 - 0.25, 0, -2.5 - 0.25 }, { 0.5, 1, 5.0 + 0.5 }),
            },
            // min and max height (distance of terrain / water to bridge floor)
            1, 1024, 
            true, // allows boats to pass under
            3000 // cost of bridge per tile
        }
    };

    const std::vector<Bridge::TypeInfo>& Bridge::types() {
        return type_infos;
    }

}