
#include "item.hpp"

namespace houseofatmos::world {

    static bool item_storable = true;

    static std::vector<Item::TypeInfo> type_infos = {
        /* Wood */ { 
            "item_name_wood", 
            &ui_icon::wood, 
            item_storable 
        },
        /* Barley */ { 
            "item_name_barley", 
            &ui_icon::barley, 
            item_storable 
        },
        /* Wheat */ { 
            "item_name_wheat", 
            &ui_icon::wheat, 
            item_storable 
        },
        /* Hops */ { 
            "item_name_hops", 
            &ui_icon::hops,
            item_storable
        },
        /* Coal */ { 
            "item_name_coal", 
            &ui_icon::coal,
            item_storable
        },
        /* CrudeOil */ { 
            "item_name_crude_oil", 
            &ui_icon::crude_oil,
            item_storable
        },
        /* Salt */ { 
            "item_name_salt", 
            &ui_icon::salt, 
            item_storable
        },
        /* IronOre */ { 
            "item_name_iron_ore", 
            &ui_icon::iron_ore, 
            item_storable
        },
        /* CopperOre */ { 
            "item_name_copper_ore", 
            &ui_icon::copper_ore, 
            item_storable
        },
        /* ZincOre */ { 
            "item_name_zinc_ore", 
            &ui_icon::zinc_ore, 
            item_storable
        },

        /* Fabric */ {
            "item_name_fabric",
            &ui_icon::fabric, 
            item_storable
        },
        /* Milk */ {
            "item_name_milk",
            &ui_icon::milk, 
            item_storable
        },
        /* Yarn */ {
            "item_name_yarn",
            &ui_icon::yarn, 
            item_storable
        },
        /* Cattle */ {
            "item_name_cattle",
            &ui_icon::cattle, 
            item_storable
        },
        /* Beef */ {
            "item_name_beef",
            &ui_icon::beef,
            item_storable
        },
        /* Flour */ {
            "item_name_flour",
            &ui_icon::flour,
            item_storable
        },
        /* Leather */ {
            "item_name_leather",
            &ui_icon::leather,
            item_storable
        },
        /* Steel */ {
            "item_name_steel",
            &ui_icon::steel,
            item_storable
        },
        /* Malt */ {
            "item_name_malt",
            &ui_icon::malt,
            item_storable
        },
        /* Planks */ {
            "item_name_planks",
            &ui_icon::planks,
            item_storable
        },
        /* Brass */ {
            "item_name_brass",
            &ui_icon::brass,
            item_storable
        },
        /* BrassRods */ {
            "item_name_brass_rods",
            &ui_icon::brass_rods,
            item_storable
        },
        /* BrassPlates */ {
            "item_name_brass_plates",
            &ui_icon::brass_plates,
            item_storable
        },
        /* BrassGears */ {
            "item_name_brass_gears",
            &ui_icon::brass_gears,
            item_storable
        },

        /* Clothing */ {
            "item_name_clothing",
            &ui_icon::clothing,
            item_storable
        },
        /* Cheese */ {
            "item_name_cheese",
            &ui_icon::cheese,
            item_storable
        },
        /* Steak */ {
            "item_name_steak",
            &ui_icon::steak,
            item_storable
        },
        /* Oil */ {
            "item_name_oil",
            &ui_icon::oil,
            item_storable
        },
        /* Beer */ {
            "item_name_beer",
            &ui_icon::beer,
            item_storable
        },
        /* Armor */ {
            "item_name_armor",
            &ui_icon::armor,
            item_storable
        },
        /* Bread */ {
            "item_name_bread",
            &ui_icon::bread,
            item_storable
        },
        /* Tools */ {
            "item_name_tools",
            &ui_icon::tools,
            item_storable
        },
        /* BrassPots */ {
            "item_name_brass_pots",
            &ui_icon::brass_pots,
            item_storable
        },
        /* OilLanterns */ {
            "item_name_oil_lanterns",
            &ui_icon::oil_lanterns,
            item_storable
        },
        /* Watches */ {
            "item_name_watches",
            &ui_icon::watches,
            item_storable
        },

        /* SteelBeams */ {
            "item_name_steel_beams",
            &ui_icon::steel_beams, 
            !item_storable 
        },
        /* PowerLooms */ {
            "item_name_power_looms",
            &ui_icon::power_looms, 
            !item_storable  
        },
        /* SteamEngines */ {
            "item_name_steam_engines",
            &ui_icon::steam_engines,
            !item_storable
        },
        /* LocomotiveFrames */ {
            "item_name_locomotive_frames",
            &ui_icon::locomotive_frames,
            !item_storable
        },
        /* Coins */ {
            "item_name_coins",
            &ui_icon::coins,
            !item_storable 
        }
    };

    const std::vector<Item::TypeInfo>& Item::types() {
        return type_infos;
    }

}