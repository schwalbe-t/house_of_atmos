
#pragma once

#include "../renderer.hpp"

namespace houseofatmos::world {
    
    struct Resource {

        struct TypeInfo {
            std::string local_name;
            engine::Model::LoadArgs model;
            engine::Texture::LoadArgs texture;

            f64 spawn_chance; // per tile
        };

        static inline const engine::Model::LoadArgs ore_model = {
            "res/resources/ore_resource.glb", Renderer::model_attribs,
            engine::FaceCulling::Enabled
        };

        static inline const std::vector<TypeInfo> types = {
            // The 'true' in each texture loader is there to flip the loaded
            // texture vertically - this is needed since GLTF uses them flipped
            /* Type::Hematite */ {
                "item_name_hematite",
                ore_model,
                (engine::Texture::LoadArgs) { 
                    "res/resources/hematite_ore.png", 
                    engine::Texture::vertical_mirror
                },
                1.0 / 1500.0 // spawn chance, on avg. approx. 43.6 in 256x256 
            },
            /* Type::Coal */ {
                "item_name_coal",
                ore_model,
                (engine::Texture::LoadArgs) { 
                    "res/resources/coal_ore.png", 
                    engine::Texture::vertical_mirror
                },
                1.0 / 1500.0 // spawn chance, on avg. approx. 43.6 in 256x256 
            }
        };

        enum Type {
            Hematite,
            Coal
        };

        static void load_resources(engine::Scene& scene) {
            for(const TypeInfo& type_info: Resource::types) {
                scene.load(engine::Model::Loader(type_info.model));
                scene.load(engine::Texture::Loader(type_info.texture));
            }
        }

        Type type;
        u8 x, z;

    };

}
