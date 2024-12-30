
#pragma once

#include "../renderer.hpp"

namespace houseofatmos::outside {

    using namespace houseofatmos;

    struct Building {

        struct TypeInfo {
            engine::Model::LoadArgs model;
            std::optional<std::string> animation;
            f64 animation_speed;
            u8 width, height; // in tiles
            f64 offset_x, offset_z; // in tiles, 0..1
            u64 cost; // in coins
        };

        static inline const std::vector<TypeInfo> types = {
            /* Type::Farmland */ {
                { "res/buildings/farmland.gltf", Renderer::model_attribs },
                std::nullopt, 0.0,
                2, 2,
                0, 0,
                500
            },
            /* Type::Mineshaft */ {
                { "res/buildings/mineshaft.gltf", Renderer::model_attribs },
                std::nullopt, 0.0,
                2, 2,
                0, 0,
                1000
            },
            /* Type::Factory */ {
                { "res/buildings/factory.gltf", Renderer::model_attribs },
                std::nullopt, 0.0,
                2, 1,
                0, 0.5,
                1000
            },
            /* Type::House */ {
                { "res/buildings/house.gltf", Renderer::model_attribs },
                "door", 0.0, // speed = 0 -> will always be the first frame
                1, 1,
                0.5, 0.5,
                500
            }
        };

        static void load_models(engine::Scene& scene) {
            for(const TypeInfo& type: Building::types) {
                scene.load(engine::Model::Loader(type.model));
            }
        }


        enum Type {
            Farmland = 0,
            Mineshaft = 1,
            Factory = 2,
            House = 3
        };

        Type type;
        u8 x, z; // in tiles relative to chunk origin


        const TypeInfo& get_type_info() const {
            return Building::types.at((size_t) this->type);
        }

    };

}