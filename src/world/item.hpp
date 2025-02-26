
#pragma once

#include <engine/nums.hpp>
#include <engine/ui.hpp>
#include "../ui_const.hpp"
#include <vector>

namespace houseofatmos::world {

    namespace ui = houseofatmos::engine::ui;


    struct Item {

        struct TypeInfo {
            std::string local_name;
            const ui::Background* icon;
            bool storable = true;
        };

        static inline const std::vector<TypeInfo> items = {
            /* Wood */ { "item_name_wood", &ui_icon::wood, true },
            /* Barley */ { "item_name_barley", &ui_icon::barley, true },
            /* Wheat */ { "item_name_wheat", &ui_icon::wheat, true },
            /* Hops */ { "item_name_hops", &ui_icon::hops, true },
            /* Coal */ { "item_name_coal", &ui_icon::coal, true },
            /* CrudeOil */ { "item_name_crude_oil", &ui_icon::crude_oil, true },
            /* Salt */ { "item_name_salt", &ui_icon::salt, true },
            /* IronOre */ { "item_name_iron_ore", &ui_icon::iron_ore, true },
            /* CopperOre */ { "item_name_copper_ore", &ui_icon::copper_ore, true },
            /* ZincOre */ { "item_name_zinc_ore", &ui_icon::zinc_ore, true },

            /* Fabric */ { "item_name_fabric", &ui_icon::fabric, true },
            /* Milk */ { "item_name_milk", &ui_icon::milk, true },
            /* Yarn */ { "item_name_yarn", &ui_icon::yarn, true },
            /* Cattle */ { "item_name_cattle", &ui_icon::cattle, true },
            /* Beef */ { "item_name_beef", &ui_icon::beef, true },
            /* Flour */ { "item_name_flour", &ui_icon::flour, true },
            /* Leather */ { "item_name_leather", &ui_icon::leather, true },
            /* Steel */ { "item_name_steel", &ui_icon::steel, true },
            /* Malt */ { "item_name_malt", &ui_icon::malt, true },
            /* Planks */ { "item_name_planks", &ui_icon::planks, true },
            /* Brass */ { "item_name_brass", &ui_icon::brass, true },
            /* BrassRods */ { "item_name_brass_rods", &ui_icon::brass_rods, true },
            /* BrassPlates */ { "item_name_brass_plates", &ui_icon::brass_plates, true },
            /* BrassGears */ { "item_name_brass_gears", &ui_icon::brass_gears, true },

            /* Clothing */ { "item_name_clothing", &ui_icon::clothing, true },
            /* Cheese */ { "item_name_cheese", &ui_icon::cheese, true },
            /* Steak */ { "item_name_steak", &ui_icon::steak, true },
            /* Oil */ { "item_name_oil", &ui_icon::oil, true },
            /* Beer */ { "item_name_beer", &ui_icon::beer, true },
            /* Armor */ { "item_name_armor", &ui_icon::armor, true },
            /* Bread */ { "item_name_bread", &ui_icon::bread, true },
            /* Tools */ { "item_name_tools", &ui_icon::tools, true },
            /* BrassPots */ { "item_name_brass_pots", &ui_icon::brass_pots, true },
            /* OilLanterns */ { "item_name_oil_lanterns", &ui_icon::oil_lanterns, true },
            /* Watches */ { "item_name_watches", &ui_icon::watches, true },

            /* SteelBeams */ { "item_name_steel_beams", &ui_icon::steel_beams, false },
            /* PowerLooms */ { "item_name_power_looms", &ui_icon::power_looms, false },
            /* SteamEngines */ { "item_name_steam_engines", &ui_icon::steam_engines, false },
            /* Coins */ { "item_name_coins", &ui_icon::coins, false }
        };

        enum Type {
            Wood, Barley, Wheat, Hops, 
            Coal, CrudeOil, Salt, IronOre, CopperOre, ZincOre,

            Fabric, Milk, Yarn, Cattle, Beef, Flour, Leather, Steel, Malt, 
            Planks, Brass, BrassRods, BrassPlates, BrassGears, 

            Clothing, Cheese, Steak, Oil, Beer, Armor, Bread, Tools, BrassPots,
            OilLanterns, Watches,

            SteelBeams, PowerLooms, SteamEngines, Coins
        };

        struct Stack {
            u8 count;
            Type item;
        };

    };

}