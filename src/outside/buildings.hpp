
#pragma once

#include "../renderer.hpp"

namespace houseofatmos::outside {

    using namespace houseofatmos;

    struct Building {

        struct TypeInfo {
            engine::Model::LoadArgs model;
            u8 width, height; // in tiles
        };

        static inline const std::vector<TypeInfo> types = {
            /* Farmland */ {
                { "res/buildings/farmland.gltf", Renderer::model_attribs },
                10, 10
            }
        };

        static void load_models(engine::Scene& scene) {
            for(const TypeInfo& type: Building::types) {
                scene.load(engine::Model::Loader(type.model));
            }
        }


        enum Type {
            Farmland = 0
        };

        Type type;
        u8 x, z; // in tiles relative to chunk origin


        const TypeInfo& get_type_info() const {
            return Building::types.at((size_t) this->type);
        }

    };

}