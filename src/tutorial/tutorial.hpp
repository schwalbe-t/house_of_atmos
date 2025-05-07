
#pragma once

#include <engine/scene.hpp>
#include "../settings.hpp"

namespace houseofatmos::world {
    struct World;
}

namespace houseofatmos::tutorial {

    std::shared_ptr<engine::Scene> create_toddler_scene(
        std::shared_ptr<world::World> world_after
    );

    std::shared_ptr<engine::Scene> create_terraform_scene(
        std::shared_ptr<world::World> world_after
    );

    std::shared_ptr<engine::Scene> create_buildings_scene(
        std::shared_ptr<world::World> world_after
    );

    std::shared_ptr<engine::Scene> create_logistics_scene(
        std::shared_ptr<world::World> world_after
    );

    std::shared_ptr<engine::Scene> create_discovery_scene(
        std::shared_ptr<world::World> world_after
    );

}