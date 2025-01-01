
#pragma once

#include "complex.hpp"
#include "../renderer.hpp"
#include "../collider.hpp"

namespace houseofatmos::outside {

    using namespace houseofatmos;

    struct Building {

        struct TypeInfo {
            engine::Model::LoadArgs model;
            std::optional<std::string> animation;
            f64 animation_speed;
            std::vector<RelCollider> colliders; // in game units
            u8 width, height; // in tiles
            f64 offset_x, offset_z; // in tiles, 0..1
            u64 cost; // in coins
        };

        static inline const std::vector<TypeInfo> types = {
            /* Type::Farmland */ {
                { "res/buildings/farmland.glb", Renderer::model_attribs },
                std::nullopt, 0.0,
                {
                    // corner fence
                    RelCollider({ -4.625, -0.5, -4.625 }, { 0.25, 1, 6.25 }),
                    RelCollider({ -4.625, -0.5, -4.625 }, { 3.25, 1, 0.25 }),
                    // long fence
                    RelCollider({  4.425, -0.5, -4.625 }, { 0.25, 1, 9.25 })
                },
                2, 2,
                0, 0,
                500
            },
            /* Type::Mineshaft */ {
                { "res/buildings/mineshaft.glb", Renderer::model_attribs },
                std::nullopt, 0.0,
                { RelCollider({ -5, -0.5, -5 }, { 10, 1, 10 }) },
                2, 2,
                0, 0,
                1000
            },
            /* Type::Windmill */ {
                { "res/buildings/windmill.glb", Renderer::model_attribs },
                "blades", 1.0,
                { RelCollider({ -3, -0.5, -3 }, { 6, 1, 6 }) },
                2, 2,
                0, 0,
                1000
            },
            /* Type::Factory */ {
                { "res/buildings/factory.glb", Renderer::model_attribs },
                std::nullopt, 0.0,
                { RelCollider({ -5, -0.5, -2.5 }, { 10, 1, 5 }) },
                2, 1,
                0, 0.5,
                1000
            },
            /* Type::House */ {
                { "res/buildings/house.glb", Renderer::model_attribs },
                "door", 0.0, // speed = 0 -> will always be the first frame
                { RelCollider({ -2.5, -0.5, -1.25 }, { 5, 1, 2.5 }) },
                1, 1,
                0.5, 0.5,
                500
            },
            /* Type::Plaza */ {
                { "res/buildings/plaza.glb", Renderer::model_attribs },
                std::nullopt, 0.0,
                { 
                    // well
                    RelCollider({ -2.50, -0.5, -2.50 }, { 5.0, 1, 5.0 }),
                    // left stand
                    RelCollider({ -7.25, -0.5, -7.25 }, { 0.5, 1, 0.5 }),
                    RelCollider({ -3.25, -0.5, -7.25 }, { 0.5, 1, 0.5 }),
                    RelCollider({ -7.25, -0.5, -3.25 }, { 0.5, 1, 0.5 }),
                    RelCollider({ -3.25, -0.5, -3.25 }, { 0.5, 1, 0.5 }),
                    RelCollider({ -7.50, -0.5, -2.25 }, { 1.0, 1, 2.0 }),
                    RelCollider({ -7.00, -0.5, -5.50 }, { 3.5, 1, 1.0 }),
                    // right stand
                    RelCollider({  6.75, -0.5, -7.25 }, { 0.5, 1, 0.5 }),
                    RelCollider({  2.75, -0.5, -7.25 }, { 0.5, 1, 0.5 }),
                    RelCollider({  6.75, -0.5, -3.25 }, { 0.5, 1, 0.5 }),
                    RelCollider({  2.75, -0.5, -3.25 }, { 0.5, 1, 0.5 }),
                    RelCollider({  1.50, -0.5, -7.50 }, { 1.0, 1, 1.0 }),
                    RelCollider({  4.50, -0.5, -5.50 }, { 2.5, 1, 1.0 })
                },
                3, 3,
                0.5, 0.5,
                5000
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
            Windmill = 2,
            Factory = 3,
            House = 4,
            Plaza = 5
        };

        Type type;
        u8 x, z; // in tiles relative to chunk origin
        std::optional<ComplexId> complex;


        const TypeInfo& get_type_info() const {
            return Building::types.at((size_t) this->type);
        }

    };

}