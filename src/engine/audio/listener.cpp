
#include <engine/audio.hpp>
#include <AL/al.h>

namespace houseofatmos::engine {

    static Vec<3> curr_position = { 0, 0, 0 };
    static f64 curr_max_source_dist = INFINITY;

    void Listener::internal_make_current() const {
        const Vec<3>& pos = this->position;
        alListener3f(AL_POSITION, pos.x(), pos.y(), pos.z());
        Vec<3> look_at = this->look_at.normalized();
        Vec<3> up = this->up.normalized();
        ALfloat orientation[] = {
            look_at.x(), look_at.y(), look_at.z(),
            up.x(), up.y(), up.z()
        };
        curr_position = this->position;
        curr_max_source_dist = this->max_speaker_distance;
    }

    const Vec<3>& Listener::internal_position() { return curr_position; }
    
    f64 Listener::internal_max_source_dist() { return curr_max_source_dist; }

}