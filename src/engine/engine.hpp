
#pragma once

#include <raylib.h>
#include "rendering.hpp"
#include "resources.hpp"

namespace houseofatmos::engine {

    void init(const char* title, int width, int height, int fps);
    bool is_running();
    void display_buffer(rendering::Surface* buffer);
    void stop();

}