
#pragma once

#include <engine/nums.hpp>
#include <engine/ui.hpp>
#include "../ui_icon.hpp"
#include <vector>

namespace houseofatmos::outside {

    struct Item {

        struct TypeInfo {
            std::string local_name;
            const ui::Background* icon;
        };

        static inline const std::vector<TypeInfo> items = {
            /* Item::Barley */ {
                "item_name_barley",
                &ui_icon::barley
            },
            /* Item::Malt */ {
                "item_name_malt",
                &ui_icon::malt
            },
            /* Item::Beer */ {
                "item_name_beer",
                &ui_icon::beer
            },
            /* Item::Wheat */ {
                "item_name_wheat",
                &ui_icon::wheat
            },
            /* Item::Flour */ {
                "item_name_flour",
                &ui_icon::flour
            },
            /* Item::Bread */ {
                "item_name_bread",
                &ui_icon::bread
            },
            /* Item::Hematite */ {
                "item_name_hematite",
                &ui_icon::hematite
            },
            /* Item::Coal */ {
                "item_name_coal",
                &ui_icon::coal
            },
            /* Item::Steel */ {
                "item_name_steel",
                &ui_icon::steel
            },
            /* Item::Armor */ {
                "item_name_armor",
                &ui_icon::armor
            },
            /* Item::Tools */ {
                "item_name_tools",
                &ui_icon::tools
            },
            /* Item::Coins */ {
                "item_name_coins",
                &ui_icon::coins
            }
        };

        enum Type {
            Barley = 0, /* -> */ Malt = 1, /* -> */ Beer = 2,
            Wheat = 3, /* -> */ Flour = 4, /* -> */ Bread = 5,
            Hematite = 6, /* and */ Coal = 7, /* -> */ Steel = 8, /* -> */ Armor = 9,
                                                                  /* or */ Tools = 10,
            Coins = 11
        };

        static inline const std::vector<Item::Type> transferrable = {
            Item::Barley, Item::Malt, Item::Beer,
            Item::Wheat, Item::Flour, Item::Bread,
            Item::Hematite, Item::Coal, Item::Steel, Item::Armor, Item::Tools
        };

        struct Stack {
            u8 count;
            Type item;
        };

    };

}