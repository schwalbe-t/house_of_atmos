
#pragma once

#include "../renderer.hpp"

namespace houseofatmos::world {
    
    struct Resource {

        struct TypeInfo {
            std::string_view local_name;
            engine::Model::LoadArgs model;
            engine::Texture::LoadArgs texture;

            f64 spawn_chance; // per tile
        };

        static const std::vector<TypeInfo>& types();

        enum Type {
            Coal,
            CrudeOil,
            Salt,
            IronOre,
            CopperOre,
            ZincOre
        };

        static void load_resources(engine::Scene& scene) {
            for(const TypeInfo& type_info: Resource::types()) {
                scene.load(type_info.model);
                scene.load(type_info.texture);
            }
        }

        Type type;
        u8 x, z;

    };

}
