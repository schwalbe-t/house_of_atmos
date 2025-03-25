
#pragma once

#include "actionmode.hpp"
#include "../terrainmap.hpp"

namespace houseofatmos::world {

    static Vec<3> tile_bounded_position(
        u64 min_x, u64 min_z, u64 max_x, u64 max_z,
        const Vec<3>& pos, const Terrain& terrain
    ) {
        Vec<3> min = Vec<3>(min_x, 0, min_z) * terrain.units_per_tile();
        Vec<3> max = Vec<3>(max_x, 0, max_z) * terrain.units_per_tile();
        Vec<3> result = Vec<3>(
            std::min(std::max(pos.x(), min.x()), max.x()),
            0,
            std::min(std::max(pos.z(), min.z()), max.z())
        );
        result.y() = terrain.elevation_at(result);
        return result;
    }

}