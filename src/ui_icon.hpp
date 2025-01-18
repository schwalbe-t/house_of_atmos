
#pragma once

#include <engine/ui.hpp>

namespace houseofatmos::ui_icon {

    namespace engine = houseofatmos::engine;
    namespace ui = engine::ui;
    using namespace engine::math;


    static inline const ui::Background terrain = (ui::Background) {
        (engine::Texture::LoadArgs) { "res/ui.png" },
        // offset, (corner size), edge size
        Vec<2>(16, 64), Vec<2>(0, 0), Vec<2>(16, 16)
    };

    static inline const ui::Background construction = (ui::Background) {
        (engine::Texture::LoadArgs) { "res/ui.png" },
        // offset, (corner size), edge size
        Vec<2>(32, 64), Vec<2>(0, 0), Vec<2>(16, 16)
    };

    static inline const ui::Background demolition = (ui::Background) {
        (engine::Texture::LoadArgs) { "res/ui.png" },
        // offset, (corner size), edge size
        Vec<2>(48, 64), Vec<2>(0, 0), Vec<2>(16, 16)
    };

    static inline const ui::Background pathing = (ui::Background) {
        (engine::Texture::LoadArgs) { "res/ui.png" },
        // offset, (corner size), edge size
        Vec<2>(64, 64), Vec<2>(0, 0), Vec<2>(16, 16)
    };


    inline void load_textures(engine::Scene& scene) {
        scene.load(engine::Texture::Loader(terrain.texture));
        scene.load(engine::Texture::Loader(construction.texture));
        scene.load(engine::Texture::Loader(demolition.texture));
        scene.load(engine::Texture::Loader(pathing.texture));
    }

}