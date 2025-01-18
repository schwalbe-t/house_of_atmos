
#pragma once

#include <engine/ui.hpp>

namespace houseofatmos::ui_font {

    namespace engine = houseofatmos::engine;
    namespace ui = engine::ui;
    using namespace engine::math;


    static inline ui::Font standard = (ui::Font) {
        (engine::Texture::LoadArgs) { "res/ui.png" },
        Vec<2>(1, 1), // offset
        5, // height
        1, // padding
        "\t "
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЮЯ"
            "абвгдежзийклмнопрстуфхцчшщъьюяѝ"
            "0123456789:–+*/%()[]{}\"'?!._<>;$#@&^,",
        { 
            5,2, 
            4,4,4,4,4,4,4,4,3,4,4,4,5,4,4,4,4,4,4,3,4,5,5,4,4,4,
            4,4,3,4,4,3,4,4,3,3,3,3,5,4,4,4,4,3,4,3,4,3,5,4,4,4,
            4,4,4,4,4,4,5,4,4,4,4,4,5,4,4,4,4,4,3,4,5,4,4,4,5,5,4,5,4,
            4,4,4,3,4,4,5,3,4,4,3,4,5,4,4,4,4,3,3,4,5,4,4,4,5,5,4,3,5,4,4,
            4,4,4,4,4,4,4,4,4,4,1,3,3,4,3,4,2,2,2,2,3,3,3,1,4,1,1,4,3,3,2,4,4,5,5,5,2
        },
        std::vector<f64>()
    };


    inline void load_textures(engine::Scene& scene) {
        scene.load(engine::Texture::Loader(standard.texture));
        standard.compute_char_offsets();
    }

}