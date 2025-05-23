
#include "ui_const.hpp"

namespace houseofatmos::ui_font {

    static const engine::Texture::LoadArgs font_texture
        = engine::Texture::LoadArgs("res/ui.png");

    static const std::string_view font_chars = "\t "
        "ABCDEFGHIJKLMNOPQRSTUVWXYZÄÖÜ"
        "abcdefghijklmnopqrstuvwxyzäöüß"
        "АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЮЯ"
        "абвгдежзийклмнопрстуфхцчшщъьюяѝ"
        "0123456789:-+*/%()[]{}\"'?!._<>;#$@&^,\\"
        "→↑↓🪙💾🗑✅∞";

    static std::vector<f64> font_char_widths = { 
        5,2, 
        4,4,4,4,4,4,4,4,3,4,4,4,5,4,4,4,4,4,4,3,4,5,5,4,4,4,4,4,4,
        4,4,3,4,4,3,4,4,3,3,3,3,5,4,4,4,4,3,4,3,4,3,5,4,4,4,4,4,4,4,
        4,4,4,4,5,4,5,4,4,4,4,4,5,4,4,4,4,4,3,4,5,4,4,4,5,5,4,5,4,
        4,4,4,3,4,4,5,3,4,4,3,4,5,4,4,4,4,3,3,4,5,4,4,4,5,5,4,3,5,4,4,
        4,4,4,4,4,4,4,4,4,4,1,3,3,4,3,4,2,2,2,2,3,3,3,1,4,1,1,4,3,3,2,4,4,5,5,5,2,3,
        7,5,5,5,5,5,5,9
    };

    ui::Font dark = ui::Font(
        font_texture,
        Vec<2>(8, 184), // offset
        5, // height
        1, // padding
        font_chars,
        font_char_widths
    );

    ui::Font bright = ui::Font(
        font_texture,
        Vec<2>(8, 192), // offset
        5, // height
        1, // padding
        font_chars,
        font_char_widths
    );


    void load_textures(engine::Scene& scene) {
        scene.load(font_texture);
        dark.compute_char_offsets();
        bright.compute_char_offsets();
    }

}