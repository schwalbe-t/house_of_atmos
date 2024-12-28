
#pragma once

#include <engine/window.hpp>
#include <engine/model.hpp>
#include "../renderer.hpp"
#include "../player.hpp"
#include "terrain.hpp"

namespace houseofatmos::outside {

    struct Outside: engine::Scene {

        Renderer renderer;
        Terrain terrain = Terrain(256, 256, 1);
        Player player;

        Outside();

        void update(const engine::Window& window) override;
        void render(const engine::Window& window) override;

    };

}