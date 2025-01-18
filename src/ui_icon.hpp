
#pragma once

#include <engine/ui.hpp>

namespace houseofatmos::ui_icon {

    namespace engine = houseofatmos::engine;
    namespace ui = engine::ui;
    using namespace engine::math;


    #define MAKE_HOA_UI_ICON(path, offset, size) (ui::Background) { \
        (engine::Texture::LoadArgs) { std::string(path) }, \
        (offset), Vec<2>(0, 0), (size) \
    }


    static inline const ui::Background terrain 
        = MAKE_HOA_UI_ICON("res/ui.png", Vec<2>(16, 64), Vec<2>(16, 16));
    static inline const ui::Background construction 
        = MAKE_HOA_UI_ICON("res/ui.png", Vec<2>(32, 64), Vec<2>(16, 16));
    static inline const ui::Background demolition 
        = MAKE_HOA_UI_ICON("res/ui.png", Vec<2>(48, 64), Vec<2>(16, 16));
    static inline const ui::Background pathing 
        = MAKE_HOA_UI_ICON("res/ui.png", Vec<2>(64, 64), Vec<2>(16, 16));

    static inline const ui::Background barley 
        = MAKE_HOA_UI_ICON("res/ui.png", Vec<2>(16, 88), Vec<2>(8, 8));
    static inline const ui::Background malt 
        = MAKE_HOA_UI_ICON("res/ui.png", Vec<2>(24, 88), Vec<2>(8, 8));
    static inline const ui::Background beer
        = MAKE_HOA_UI_ICON("res/ui.png", Vec<2>(32, 88), Vec<2>(8, 8));
    static inline const ui::Background wheat 
        = MAKE_HOA_UI_ICON("res/ui.png", Vec<2>(16, 96), Vec<2>(8, 8));
    static inline const ui::Background flour 
        = MAKE_HOA_UI_ICON("res/ui.png", Vec<2>(24, 96), Vec<2>(8, 8));
    static inline const ui::Background bread
        = MAKE_HOA_UI_ICON("res/ui.png", Vec<2>(32, 96), Vec<2>(8, 8));
    static inline const ui::Background hematite 
        = MAKE_HOA_UI_ICON("res/ui.png", Vec<2>(16, 104), Vec<2>(8, 8));
    static inline const ui::Background coal 
        = MAKE_HOA_UI_ICON("res/ui.png", Vec<2>(24, 104), Vec<2>(8, 8));
    static inline const ui::Background steel
        = MAKE_HOA_UI_ICON("res/ui.png", Vec<2>(32, 104), Vec<2>(8, 8));
    static inline const ui::Background armor
        = MAKE_HOA_UI_ICON("res/ui.png", Vec<2>(40, 104), Vec<2>(8, 8));
    static inline const ui::Background tools
        = MAKE_HOA_UI_ICON("res/ui.png", Vec<2>(48, 104), Vec<2>(8, 8));
    static inline const ui::Background coins
        = MAKE_HOA_UI_ICON("res/ui.png", Vec<2>(16, 112), Vec<2>(8, 8));


    inline void load_textures(engine::Scene& scene) {
        scene.load(engine::Texture::Loader(terrain.texture));
        scene.load(engine::Texture::Loader(construction.texture));
        scene.load(engine::Texture::Loader(demolition.texture));
        scene.load(engine::Texture::Loader(pathing.texture));
        
        scene.load(engine::Texture::Loader(barley.texture));
        scene.load(engine::Texture::Loader(malt.texture));
        scene.load(engine::Texture::Loader(beer.texture));
        scene.load(engine::Texture::Loader(wheat.texture));
        scene.load(engine::Texture::Loader(flour.texture));
        scene.load(engine::Texture::Loader(bread.texture));
        scene.load(engine::Texture::Loader(hematite.texture));
        scene.load(engine::Texture::Loader(coal.texture));
        scene.load(engine::Texture::Loader(steel.texture));
        scene.load(engine::Texture::Loader(armor.texture));
        scene.load(engine::Texture::Loader(tools.texture));
        scene.load(engine::Texture::Loader(coins.texture));
    }

}