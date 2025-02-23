
#pragma once

#include <engine/scene.hpp>
#include "../settings.hpp"

namespace houseofatmos::world {
    struct World;
}

namespace houseofatmos::tutorial {

    std::shared_ptr<engine::Scene> create_scene(
        std::shared_ptr<world::World>&& world_after
    );

}