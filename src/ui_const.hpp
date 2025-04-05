
#pragma once

#include <engine/ui.hpp>



namespace houseofatmos::ui_font {

    namespace engine = houseofatmos::engine;
    namespace ui = engine::ui;
    using namespace engine::math;


    extern ui::Font dark;

    extern ui::Font bright;


    void load_textures(engine::Scene& scene);

}



namespace houseofatmos::ui_background {

    namespace engine = houseofatmos::engine;
    namespace ui = engine::ui;
    using namespace engine::math;


    /*
       Backgrounds have the following structure on the texture:
    
       +-----+-----+-----+  #1 = corner
       | #1  | #2  | #1  |  #2 = edge
       |     |     |     |  #3 = actual background of the element
       +-----+-----+-----+
       | #2  | #3  | #2  |  The edges and background are repeated to fill
       |     |     |     |  the size of the actual element.
       +-----+-----+-----+  Each background is defined by a texture,
       | #1  | #2  | #1  |  the position of this square in said texture,
       |     |     |     |  the size of each corner and the size of each edge.
       +-----+-----+-----+
    */


    static inline const ui::Background scroll_horizontal = (ui::Background) {
        engine::Texture::LoadArgs("res/ui.png"),
        // offset, corner size, edge size
        Vec<2>(8, 8), Vec<2>(8, 8), Vec<2>(32, 32)
    };

    static inline const ui::Background scroll_vertical = (ui::Background) {
        engine::Texture::LoadArgs("res/ui.png"),
        // offset, corner size, edge size
        Vec<2>(64, 8), Vec<2>(8, 8), Vec<2>(32, 32)
    };

    static inline const ui::Background border = (ui::Background) {
        engine::Texture::LoadArgs("res/ui.png"),
        // offset, corner size, edge size
        Vec<2>(120, 8), Vec<2>(8, 8), Vec<2>(32, 32)
    };

    static inline const ui::Background border_hovering = (ui::Background) {
        engine::Texture::LoadArgs("res/ui.png"),
        // offset, corner size, edge size
        Vec<2>(176, 8), Vec<2>(8, 8), Vec<2>(32, 32)
    };

    static inline const ui::Background border_selected = (ui::Background) {
        engine::Texture::LoadArgs("res/ui.png"),
        // offset, corner size, edge size
        Vec<2>(232, 8), Vec<2>(8, 8), Vec<2>(32, 32)
    };

    static inline const ui::Background border_dark = (ui::Background) {
        engine::Texture::LoadArgs("res/ui.png"),
        // offset, corner size, edge size
        Vec<2>(288, 8), Vec<2>(8, 8), Vec<2>(32, 32)
    };

    static inline const ui::Background button = (ui::Background) {
        engine::Texture::LoadArgs("res/ui.png"),
        // offset, corner size, edge size
        Vec<2>(344, 8), Vec<2>(8, 8), Vec<2>(32, 32)
    };

    static inline const ui::Background button_select = (ui::Background) {
        engine::Texture::LoadArgs("res/ui.png"),
        // offset, corner size, edge size
        Vec<2>(400, 8), Vec<2>(8, 8), Vec<2>(32, 32)
    };

    static inline const ui::Background note = (ui::Background) {
        engine::Texture::LoadArgs("res/ui.png"),
        // offset, corner size, edge size
        Vec<2>(456, 8), Vec<2>(8, 8), Vec<2>(32, 32)
    };

    static inline const ui::Background note_error = (ui::Background) {
        engine::Texture::LoadArgs("res/ui.png"),
        // offset, corner size, edge size
        Vec<2>(512, 8), Vec<2>(8, 8), Vec<2>(32, 32)
    };


    inline void load_textures(engine::Scene& scene) {
        scene.load(scroll_horizontal.texture);
        scene.load(scroll_vertical.texture);
        scene.load(border.texture);
        scene.load(border_hovering.texture);
        scene.load(border_selected.texture);
        scene.load(button.texture);
        scene.load(button_select.texture);
    }

}



namespace houseofatmos::ui_icon {

    namespace engine = houseofatmos::engine;
    namespace ui = engine::ui;
    using namespace engine::math;


    #define MAKE_HOA_UI_ICON(offset, size) (ui::Background) { \
        engine::Texture::LoadArgs("res/ui.png"), \
        (offset), Vec<2>(0, 0), (size) \
    }


    static inline const ui::Background terraforming 
        = MAKE_HOA_UI_ICON(Vec<2>(16, 64), Vec<2>(16, 16));
    static inline const ui::Background construction 
        = MAKE_HOA_UI_ICON(Vec<2>(32, 64), Vec<2>(16, 16));
    static inline const ui::Background demolition 
        = MAKE_HOA_UI_ICON(Vec<2>(48, 64), Vec<2>(16, 16));
    static inline const ui::Background pathing 
        = MAKE_HOA_UI_ICON(Vec<2>(64, 64), Vec<2>(16, 16));
    static inline const ui::Background bridging 
        = MAKE_HOA_UI_ICON(Vec<2>(80, 64), Vec<2>(16, 16));
    static inline const ui::Background tracking 
        = MAKE_HOA_UI_ICON(Vec<2>(96, 64), Vec<2>(16, 16));

    static inline const ui::Background terrain_flatten 
        = MAKE_HOA_UI_ICON(Vec<2>(128, 64), Vec<2>(16, 16));
    static inline const ui::Background terrain_raise 
        = MAKE_HOA_UI_ICON(Vec<2>(144, 64), Vec<2>(16, 16));
    static inline const ui::Background terrain_lower
        = MAKE_HOA_UI_ICON(Vec<2>(160, 64), Vec<2>(16, 16));
    static inline const ui::Background terrain_vertex
        = MAKE_HOA_UI_ICON(Vec<2>(176, 64), Vec<2>(8, 8));

    static const ui::Background track_any_icon 
        = MAKE_HOA_UI_ICON(Vec<2>(192, 64), Vec<2>(8, 8));
    static const Vec<2> track_dir_icon_first = Vec<2>(0, -1); // in ndc
    static const std::vector<ui::Background> track_dir_icons = {
        MAKE_HOA_UI_ICON(Vec<2>(200, 64), Vec<2>(8, 8)),
        MAKE_HOA_UI_ICON(Vec<2>(208, 64), Vec<2>(8, 8)),
        MAKE_HOA_UI_ICON(Vec<2>(216, 64), Vec<2>(8, 8)),
        MAKE_HOA_UI_ICON(Vec<2>(224, 64), Vec<2>(8, 8)),
        MAKE_HOA_UI_ICON(Vec<2>(232, 64), Vec<2>(8, 8)),
        MAKE_HOA_UI_ICON(Vec<2>(240, 64), Vec<2>(8, 8)),
        MAKE_HOA_UI_ICON(Vec<2>(248, 64), Vec<2>(8, 8)),
        MAKE_HOA_UI_ICON(Vec<2>(256, 64), Vec<2>(8, 8))
    };

    static inline const ui::Background wood 
        = MAKE_HOA_UI_ICON(Vec<2>(16, 88), Vec<2>(8, 8));
    static inline const ui::Background barley 
        = MAKE_HOA_UI_ICON(Vec<2>(24, 88), Vec<2>(8, 8));
    static inline const ui::Background wheat 
        = MAKE_HOA_UI_ICON(Vec<2>(32, 88), Vec<2>(8, 8));
    static inline const ui::Background hops 
        = MAKE_HOA_UI_ICON(Vec<2>(40, 88), Vec<2>(8, 8));
    static inline const ui::Background coal 
        = MAKE_HOA_UI_ICON(Vec<2>(56, 88), Vec<2>(8, 8));
    static inline const ui::Background crude_oil 
        = MAKE_HOA_UI_ICON(Vec<2>(64, 88), Vec<2>(8, 8));
    static inline const ui::Background salt 
        = MAKE_HOA_UI_ICON(Vec<2>(72, 88), Vec<2>(8, 8));
    static inline const ui::Background iron_ore
        = MAKE_HOA_UI_ICON(Vec<2>(80, 88), Vec<2>(8, 8));
    static inline const ui::Background copper_ore
        = MAKE_HOA_UI_ICON(Vec<2>(88, 88), Vec<2>(8, 8));
    static inline const ui::Background zinc_ore
        = MAKE_HOA_UI_ICON(Vec<2>(96, 88), Vec<2>(8, 8));
    static inline const ui::Background fabric
        = MAKE_HOA_UI_ICON(Vec<2>(16, 96), Vec<2>(8, 8));
    static inline const ui::Background milk
        = MAKE_HOA_UI_ICON(Vec<2>(24, 96), Vec<2>(8, 8));
    static inline const ui::Background yarn
        = MAKE_HOA_UI_ICON(Vec<2>(32, 96), Vec<2>(8, 8));
    static inline const ui::Background cattle
        = MAKE_HOA_UI_ICON(Vec<2>(40, 96), Vec<2>(8, 8));
    static inline const ui::Background beef
        = MAKE_HOA_UI_ICON(Vec<2>(48, 96), Vec<2>(8, 8));
    static inline const ui::Background flour
        = MAKE_HOA_UI_ICON(Vec<2>(56, 96), Vec<2>(8, 8));
    static inline const ui::Background leather
        = MAKE_HOA_UI_ICON(Vec<2>(64, 96), Vec<2>(8, 8));
    static inline const ui::Background steel
        = MAKE_HOA_UI_ICON(Vec<2>(72, 96), Vec<2>(8, 8));
    static inline const ui::Background malt
        = MAKE_HOA_UI_ICON(Vec<2>(80, 96), Vec<2>(8, 8));
    static inline const ui::Background planks
        = MAKE_HOA_UI_ICON(Vec<2>(88, 96), Vec<2>(8, 8));
    static inline const ui::Background brass
        = MAKE_HOA_UI_ICON(Vec<2>(96, 96), Vec<2>(8, 8));
    static inline const ui::Background brass_rods
        = MAKE_HOA_UI_ICON(Vec<2>(104, 96), Vec<2>(8, 8));
    static inline const ui::Background brass_plates
        = MAKE_HOA_UI_ICON(Vec<2>(112, 96), Vec<2>(8, 8));
    static inline const ui::Background brass_gears
        = MAKE_HOA_UI_ICON(Vec<2>(120, 96), Vec<2>(8, 8));
    static inline const ui::Background clothing
        = MAKE_HOA_UI_ICON(Vec<2>(16, 104), Vec<2>(8, 8));
    static inline const ui::Background cheese
        = MAKE_HOA_UI_ICON(Vec<2>(24, 104), Vec<2>(8, 8));
    static inline const ui::Background steak
        = MAKE_HOA_UI_ICON(Vec<2>(32, 104), Vec<2>(8, 8));
    static inline const ui::Background oil
        = MAKE_HOA_UI_ICON(Vec<2>(40, 104), Vec<2>(8, 8));
    static inline const ui::Background beer
        = MAKE_HOA_UI_ICON(Vec<2>(48, 104), Vec<2>(8, 8));
    static inline const ui::Background armor
        = MAKE_HOA_UI_ICON(Vec<2>(56, 104), Vec<2>(8, 8));
    static inline const ui::Background bread
        = MAKE_HOA_UI_ICON(Vec<2>(64, 104), Vec<2>(8, 8));
    static inline const ui::Background tools
        = MAKE_HOA_UI_ICON(Vec<2>(72, 104), Vec<2>(8, 8));
    static inline const ui::Background brass_pots
        = MAKE_HOA_UI_ICON(Vec<2>(80, 104), Vec<2>(8, 8));
    static inline const ui::Background oil_lanterns
        = MAKE_HOA_UI_ICON(Vec<2>(88, 104), Vec<2>(8, 8));
    static inline const ui::Background watches
        = MAKE_HOA_UI_ICON(Vec<2>(96, 104), Vec<2>(8, 8));
    static inline const ui::Background steel_beams
        = MAKE_HOA_UI_ICON(Vec<2>(16, 112), Vec<2>(8, 8));
    static inline const ui::Background power_looms
        = MAKE_HOA_UI_ICON(Vec<2>(24, 112), Vec<2>(8, 8));
    static inline const ui::Background steam_engines
        = MAKE_HOA_UI_ICON(Vec<2>(32, 112), Vec<2>(8, 8));
    static inline const ui::Background coins
        = MAKE_HOA_UI_ICON(Vec<2>(40, 112), Vec<2>(8, 8));

    static inline const ui::Background farmland
        = MAKE_HOA_UI_ICON(Vec<2>(16, 128), Vec<2>(16, 16));
    static inline const ui::Background mineshaft
        = MAKE_HOA_UI_ICON(Vec<2>(32, 128), Vec<2>(16, 16));
    static inline const ui::Background windmill
        = MAKE_HOA_UI_ICON(Vec<2>(48, 128), Vec<2>(16, 16));
    static inline const ui::Background factory
        = MAKE_HOA_UI_ICON(Vec<2>(64, 128), Vec<2>(16, 16));
    static inline const ui::Background house
        = MAKE_HOA_UI_ICON(Vec<2>(80, 128), Vec<2>(16, 16));
    static inline const ui::Background stable
        = MAKE_HOA_UI_ICON(Vec<2>(96, 128), Vec<2>(16, 16));
    static inline const ui::Background plaza
        = MAKE_HOA_UI_ICON(Vec<2>(112, 128), Vec<2>(16, 16));
    static inline const ui::Background mansion
        = MAKE_HOA_UI_ICON(Vec<2>(128, 128), Vec<2>(16, 16));
    static inline const ui::Background pasture
        = MAKE_HOA_UI_ICON(Vec<2>(144, 128), Vec<2>(16, 16));
    static inline const ui::Background tree_farm
        = MAKE_HOA_UI_ICON(Vec<2>(160, 128), Vec<2>(16, 16));
    static inline const ui::Background depot
        = MAKE_HOA_UI_ICON(Vec<2>(176, 128), Vec<2>(16, 16));
    static inline const ui::Background ship_yard
        = MAKE_HOA_UI_ICON(Vec<2>(192, 128), Vec<2>(16, 16));
    static inline const ui::Background storage
        = MAKE_HOA_UI_ICON(Vec<2>(208, 128), Vec<2>(16, 16));
    static inline const ui::Background commissary_works
        = MAKE_HOA_UI_ICON(Vec<2>(224, 128), Vec<2>(16, 16));
    static inline const ui::Background manufacturing_works
        = MAKE_HOA_UI_ICON(Vec<2>(240, 128), Vec<2>(16, 16));
    static inline const ui::Background cloth_works
        = MAKE_HOA_UI_ICON(Vec<2>(256, 128), Vec<2>(16, 16));

    static inline const ui::Background wooden_bridge
        = MAKE_HOA_UI_ICON(Vec<2>(16, 144), Vec<2>(16, 16));
    static inline const ui::Background stone_bridge
        = MAKE_HOA_UI_ICON(Vec<2>(32, 144), Vec<2>(16, 16));
    static inline const ui::Background metal_bridge
        = MAKE_HOA_UI_ICON(Vec<2>(48, 144), Vec<2>(16, 16));

    static inline const ui::Background round_carriage
        = MAKE_HOA_UI_ICON(Vec<2>(112, 144), Vec<2>(16, 16));
    static inline const ui::Background passenger_carriage
        = MAKE_HOA_UI_ICON(Vec<2>(128, 144), Vec<2>(16, 16));

    static inline const ui::Background basic_locomotive
        = MAKE_HOA_UI_ICON(Vec<2>(208, 144), Vec<2>(16, 16));
    static inline const ui::Background small_locomotive
        = MAKE_HOA_UI_ICON(Vec<2>(224, 144), Vec<2>(16, 16));
    static inline const ui::Background tram
        = MAKE_HOA_UI_ICON(Vec<2>(240, 144), Vec<2>(16, 16));

    static inline const ui::Background sail_boat
        = MAKE_HOA_UI_ICON(Vec<2>(304, 144), Vec<2>(16, 16));

    static inline const ui::Background map_marker_player
        = MAKE_HOA_UI_ICON(Vec<2>(16, 168), Vec<2>(5, 6));
    static inline const ui::Background map_marker_selected
        = MAKE_HOA_UI_ICON(Vec<2>(24, 168), Vec<2>(5, 6));
    static inline const ui::Background map_marker_carriage
        = MAKE_HOA_UI_ICON(Vec<2>(32, 168), Vec<2>(5, 6));
    static inline const ui::Background map_marker_train
        = MAKE_HOA_UI_ICON(Vec<2>(40, 168), Vec<2>(5, 6));
    static inline const ui::Background map_marker_boat
        = MAKE_HOA_UI_ICON(Vec<2>(48, 168), Vec<2>(5, 6));
    static inline const ui::Background map_marker_agent_lost
        = MAKE_HOA_UI_ICON(Vec<2>(56, 168), Vec<2>(5, 6));
    static inline const ui::Background map_marker_personal_horse
        = MAKE_HOA_UI_ICON(Vec<2>(64, 168), Vec<2>(5, 6));

    static inline const ui::Background earth
        = MAKE_HOA_UI_ICON(Vec<2>(96, 240), Vec<2>(16, 16));


    inline void load_textures(engine::Scene& scene) {
        scene.load(terraforming.texture); // 'res/ui.png'
        // all icons use the same texture, so I won't repeat this here
    }

}



namespace houseofatmos::ui_const {

    inline void load_all(engine::Scene& scene) {
        ui_font::load_textures(scene);
        ui_background::load_textures(scene);
        ui_icon::load_textures(scene);
    }

}