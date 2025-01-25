
#pragma once

#include <engine/ui.hpp>

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
        (engine::Texture::LoadArgs) { "res/ui.png" },
        // offset, corner size, edge size
        Vec<2>(8, 8), Vec<2>(8, 8), Vec<2>(32, 32)
    };

    static inline const ui::Background scroll_vertical = (ui::Background) {
        (engine::Texture::LoadArgs) { "res/ui.png" },
        // offset, corner size, edge size
        Vec<2>(64, 8), Vec<2>(8, 8), Vec<2>(32, 32)
    };

    static inline const ui::Background border = (ui::Background) {
        (engine::Texture::LoadArgs) { "res/ui.png" },
        // offset, corner size, edge size
        Vec<2>(120, 8), Vec<2>(8, 8), Vec<2>(32, 32)
    };

    static inline const ui::Background border_hovering = (ui::Background) {
        (engine::Texture::LoadArgs) { "res/ui.png" },
        // offset, corner size, edge size
        Vec<2>(176, 8), Vec<2>(8, 8), Vec<2>(32, 32)
    };

    static inline const ui::Background border_selected = (ui::Background) {
        (engine::Texture::LoadArgs) { "res/ui.png" },
        // offset, corner size, edge size
        Vec<2>(232, 8), Vec<2>(8, 8), Vec<2>(32, 32)
    };

    static inline const ui::Background button = (ui::Background) {
        (engine::Texture::LoadArgs) { "res/ui.png" },
        // offset, corner size, edge size
        Vec<2>(288, 8), Vec<2>(8, 8), Vec<2>(32, 32)
    };

    static inline const ui::Background button_select = (ui::Background) {
        (engine::Texture::LoadArgs) { "res/ui.png" },
        // offset, corner size, edge size
        Vec<2>(344, 8), Vec<2>(8, 8), Vec<2>(32, 32)
    };


    inline void load_textures(engine::Scene& scene) {
        scene.load(engine::Texture::Loader(scroll_horizontal.texture));
        scene.load(engine::Texture::Loader(scroll_vertical.texture));
        scene.load(engine::Texture::Loader(border.texture));
        scene.load(engine::Texture::Loader(border_hovering.texture));
        scene.load(engine::Texture::Loader(border_selected.texture));
        scene.load(engine::Texture::Loader(button.texture));
        scene.load(engine::Texture::Loader(button_select.texture));
    }

}