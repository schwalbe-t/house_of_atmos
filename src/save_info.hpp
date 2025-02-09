
#pragma once

#include <engine/arena.hpp>
#include "settings.hpp"
#include <functional>

namespace houseofatmos {

    struct SaveInfo {
        Settings* settings;
        std::string* save_path;
        std::function<engine::Arena ()> serialize;
    };

}