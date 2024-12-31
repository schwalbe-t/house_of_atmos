
#pragma once

#include <engine/math.hpp>

namespace houseofatmos {

    using namespace houseofatmos::engine::math;


    struct Collider {
        
        Vec<3> offset;
        Vec<3> size;
    
        Collider(Vec<3> offset, Vec<3> size) {
            this->offset = offset;
            this->size = size;
        }

        bool inside_collider(
            const Vec<3>& collider_relative_to, const Vec<3>& position
        ) const {
            Vec<3> coll_start = collider_relative_to + this->offset;
            Vec<3> coll_end = coll_start + this->size;
            return coll_start.x() < position.x() && position.x() < coll_end.x()
                && coll_start.y() < position.y() && position.y() < coll_end.y()
                && coll_start.z() < position.z() && position.z() < coll_end.z();
        }
    
    };

}