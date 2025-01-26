
#pragma once

#include <engine/ui.hpp>

namespace houseofatmos::ui_font {

    namespace engine = houseofatmos::engine;
    namespace ui = engine::ui;
    using namespace engine::math;


    extern ui::Font standard;


    void load_textures(engine::Scene& scene);

}