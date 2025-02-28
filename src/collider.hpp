
#pragma once

#include <engine/math.hpp>

namespace houseofatmos {

    using namespace houseofatmos::engine::math;


    struct AbsCollider {

        Vec<3> start;
        Vec<3> end;

        bool contains(const Vec<3>& pos) const {
            return this->start.x() < pos.x() && pos.x() < this->end.x()
                && this->start.y() < pos.y() && pos.y() < this->end.y()
                && this->start.z() < pos.z() && pos.z() < this->end.z();
        }

        bool collides_with(const AbsCollider& coll) const {
            return this->end.x() > coll.start.x()
                && coll.end.x() > this->start.x()
                && this->end.y() > coll.start.y()
                && coll.end.y() > this->start.y()
                && this->end.z() > coll.start.z()
                && coll.end.z() > this->start.z();
        }
        
    };


    struct RelCollider {
        
        Vec<3> offset;
        Vec<3> size;
    
        constexpr RelCollider(Vec<3> offset, Vec<3> size): 
            offset(offset), size(size) {}

        static constexpr RelCollider none() {
            Vec<3> inf = Vec<3>(INFINITY, INFINITY, INFINITY);
            return RelCollider(inf, inf);
        }

        AbsCollider at(const Vec<3>& position) const {
            Vec<3> start = position + this->offset;
            return (AbsCollider) { start, start + this->size };
        }
    
    };

}