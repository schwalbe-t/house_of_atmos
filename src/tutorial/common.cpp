
#include "common.hpp"

namespace houseofatmos::tutorial {

    void for_line(
        u64 min_x, u64 min_z, u64 max_x, u64 max_z,
        std::function<void (u64, u64)> action
    ) {
        u64 x = min_x;
        u64 z = min_z;
        for(;;) {
            action(x, z);
            if(x != max_x) { x += 1; }
            else if(z != max_z) { z += 1; }
            else { break; }
        }
    }

    std::function<void (u64, u64)> remove_foliage_from(
        world::Terrain& terrain
    ) {
        return [terrain = &terrain](u64 x, u64 z) {
            terrain->remove_foliage_at(x, z);
        };
    }

    std::function<void (u64, u64)> set_path_at(world::Terrain& terrain) {
        return [terrain = &terrain](u64 x, u64 z) {
            terrain->set_path_at(x, z);
        };
    }

}