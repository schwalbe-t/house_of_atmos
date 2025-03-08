
#pragma once

#include "../renderer.hpp"
#include "../collider.hpp"

namespace houseofatmos::world {

    using namespace houseofatmos;


    struct Foliage {

        using ProbabilityFunc = f64 (*)(f64 nmap_val);

        struct TypeInfo {
            engine::Model::LoadArgs model;
            RelCollider collider; // in game units
            u64 attempt_count; // number of spawn attempts per tile
            // spawn chance (0-1) with 0 height difference on the tile
            // decreases to 0 on stone with higher slopes, is 0 on sand
            ProbabilityFunc spawn_chance;
        };
        
        static const std::vector<TypeInfo>& types();
        

        enum Type {
            Grass = 0,
            Tree = 1
        };

        Type type;
        u8 x, z; // position relative to chunk origin in game units
        f32 y; // position relative to chunk origin / world in game units
        f32 rotation;


        static void load_models(engine::Scene& scene) {
            for(const TypeInfo& type_model: Foliage::types()) {
                scene.load(type_model.model);
            }
        }

        TypeInfo get_type_info() const {
            return Foliage::types().at((size_t) this->type);
        }

    };

}