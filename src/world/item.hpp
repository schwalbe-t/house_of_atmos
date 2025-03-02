
#pragma once

#include <engine/nums.hpp>
#include <engine/ui.hpp>
#include "../ui_const.hpp"
#include <vector>

namespace houseofatmos::world {

    namespace ui = houseofatmos::engine::ui;


    struct Item {

        struct TypeInfo {
            std::string_view local_name;
            const ui::Background* icon;
            bool storable = true;
        };

        static const std::vector<TypeInfo>& types();

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