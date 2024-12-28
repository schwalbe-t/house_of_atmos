
#pragma once

#include "../renderer.hpp"

namespace houseofatmos::outside {

    using namespace houseofatmos;


    struct Foliage {

        struct TypeInfo {
            engine::Model::LoadArgs model;
            u64 attempt_count; // number of spawn attempts per tile
            // spawn chance (0-1) with 0 height difference on the tile
            // decreases to 0 on stone with higher slopes, is 0 on sand
            f64 spawn_chance; 
        };

        static inline const std::vector<TypeInfo> types = {
            /* Grass */ {
                { "res/foliage/grass.gltf", Renderer::model_attribs },
                10,
                1
            },
            /* Tree */ {
                { "res/foliage/tree.gltf", Renderer::model_attribs },
                1,
                0.25
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