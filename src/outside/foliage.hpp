
#pragma once

#include "../renderer.hpp"

namespace houseofatmos::outside {

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

        static inline const std::vector<TypeInfo> types = {
            /* Grass */ {
                { 
                    "res/foliage/grass.glb", Renderer::model_attribs,
                    engine::FaceCulling::Disabled
                },
                RelCollider::none(),
                10,
                [](f64 n) { (void) n; return 1.0; } // chance is always 0.0
            },
            /* Tree */ {
                {
                    "res/foliage/tree.glb", Renderer::model_attribs,
                    engine::FaceCulling::Disabled
                },
                RelCollider({ -5.0/16, -0.5, -5.0/16 }, { 10.0/16, 1, 10.0/16 }),
                1,
                [](f64 n) { 
                    return n < 0.4? 0.05  // 40% of area =>  5% chance
                         : n < 0.6? 0.50  // 20% of area => 50% chance
                         :          0.80; // 40% of area => 80% chance
                }
            }
        };


        enum Type {
            Grass = 0,
            Tree = 1
        };

        Type type;
        u8 x, z; // position relative to chunk origin in game units
        f32 y; // position relative to chunk origin / world in game units
        f32 rotation;


        static void load_models(engine::Scene& scene) {
            for(const TypeInfo& type_model: Foliage::types) {
                scene.load(engine::Model::Loader(type_model.model));
            }
        }

        TypeInfo get_type_info() const {
            return Foliage::types.at((size_t) this->type);
        }

    };

}