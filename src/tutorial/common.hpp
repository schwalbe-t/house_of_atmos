
#include "../world/scene.hpp"

namespace houseofatmos::tutorial {

    static inline const f64 father_v_pitch = 1.8;
    static inline const f64 father_v_speed = 2.0;
    static inline const f64 prompt_v_pitch = 2.0;
    static inline const f64 prompt_v_speed = 2.0;


    void for_line(
        u64 min_x, u64 min_z, u64 max_x, u64 max_z,
        std::function<void (u64, u64)> action
    );

    std::function<void (u64, u64)> remove_foliage_from(world::Terrain& terrain);

    std::function<void (u64, u64)> set_path_at(world::Terrain& terrain);

}