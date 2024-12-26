
#pragma once

#include "../renderer.hpp"

namespace houseofatmos::outside {

    using namespace houseofatmos;


    struct Foliage {

        static inline const std::vector<engine::Model::LoadArgs> type_models = {
            /* Grass */ {
                "res/foliage/grass.gltf", Renderer::model_attribs
            }
        };


        enum Type {
            Grass = 0
        };

        Type type;
        u8 x, z; // position relative to chunk origin in game units
        f32 rotation;


        static void load_models(engine::Scene& scene) {
            for(const engine::Model::LoadArgs& type_model: Foliage::type_models) {
                scene.load(engine::Model::Loader(type_model));
            }
        }

        engine::Model::LoadArgs get_type_model() const {
            return Foliage::type_models.at((size_t) this->type);
        }

    };

}