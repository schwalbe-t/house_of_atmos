
#include "ui_const.hpp"

namespace houseofatmos::ui_font {

    static const engine::Texture::LoadArgs font_texture
        = (engine::Texture::LoadArgs) { "res/ui.png" };

    static const std::string font_chars = "\t "
        "ABCDEFGHIJKLMNOPQRSTUVWXYZÄÖÜ"
        "abcdefghijklmnopqrstuvwxyzäöüß"
        "АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЮЯ"
        "абвгдежзийклмнопрстуфхцчшщъьюяѝ"
        "0123456789:-+*/%()[]{}\"'?!._<>;#$@&^,"
        "→↑↓🪙💾🗑";

    static const std::vector<f64> font_char_widths = { 
        5,2, 
        4,4,4,4,4,4,4,4,3,4,4,4,5,4,4,4,4,4,4,3,4,5,5,4,4,4,4,4,4,
        4,4,3,4,4,3,4,4,3,3,3,3,5,4,4,4,4,3,4,3,4,3,5,4,4,4,4,4,4,4,
        4,4,4,4,4,4,5,4,4,4,4,4,5,4,4,4,4,4,3,4,5,4,4,4,5,5,4,5,4,
        4,4,4,3,4,4,5,3,4,4,3,4,5,4,4,4,4,3,3,4,5,4,4,4,5,5,4,3,5,4,4,
        4,4,4,4,4,4,4,4,4,4,1,3,3,4,3,4,2,2,2,2,3,3,3,1,4,1,1,4,3,3,2,4,4,5,5,5,2,
        7,5,5,5,5,5
    };

    ui::Font dark = (ui::Font) {
        font_texture,
        Vec<2>(8, 184), // offset
        5, // height
        1, // padding
        font_chars,
        font_char_widths,
        std::vector<f64>()
    };

    ui::Font bright = (ui::Font) {
        font_texture,
        Vec<2>(8, 192), // offset
        5, // height
        1, // padding
        font_chars,
        font_char_widths,
        std::vector<f64>()
    };


    void load_textures(engine::Scene& scene) {
        scene.load(engine::Texture::Loader(font_texture));
        dark.compute_char_offsets();
        bright.compute_char_offsets();
    }

}