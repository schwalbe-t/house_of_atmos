
#pragma once

#include "complex.hpp"
#include "../renderer.hpp"
#include "../collider.hpp"
#include "../ui_const.hpp"

namespace houseofatmos::outside {

    using namespace houseofatmos;

    struct Building {

        struct TypeInfo {
            std::string local_name;
            const ui::Background* icon;
            engine::Model::LoadArgs model;
            std::optional<std::string> animation;
            f64 animation_speed;
            std::vector<RelCollider> colliders; // in game units
            u8 width, height; // in tiles
            u64 cost; // in coins
            bool destructible;
            u64 workers;
            u64 residents;

            void render_buildings(
                const engine::Window& window, engine::Scene& scene,
                const Renderer& renderer,
                std::span<const Mat<4>> instances,
                bool wireframe = false,
                const engine::Texture* override_texture = nullptr
            ) const {
                engine::Model& model = scene.get<engine::Model>(this->model);
                if(!this->animation.has_value()) {
                    renderer.render(
                        model, instances, wireframe, override_texture
                    );
                    return;
                }
                const engine::Animation& anim = model
                    .animation(*this->animation);
                f64 timestamp = fmod(
                    window.time() * this->animation_speed, anim.length()
                );
                renderer.render(
                    model, instances, 
                    anim, timestamp, 
                    wireframe, override_texture
                );
            }
        };

        static inline const std::vector<TypeInfo> types = {
            /* Type::Farmland */ {
                "building_name_farmland",
                &ui_icon::farmland,
                { "res/buildings/farmland.glb", Renderer::model_attribs },
                std::nullopt, 0.0,
                {
                    // corner fence
                    RelCollider({ -4.625, -0.5, -4.625 }, { 0.25, 1, 6.25 }),
                    RelCollider({ -4.625, -0.5, -4.625 }, { 3.25, 1, 0.25 }),
                    // long fence
                    RelCollider({  4.425, -0.5, -4.625 }, { 0.25, 1, 9.25 })
                },
                2, 2, // size
                500, // building cost
                true, // may be destroyed
                5, // workers
                0 // residents
            },
            /* Type::Mineshaft */ {
                "building_name_mineshaft",
                &ui_icon::mineshaft,
                { "res/buildings/mineshaft.glb", Renderer::model_attribs },
                std::nullopt, 0.0,
                { RelCollider({ -5, -0.5, -5 }, { 10, 1, 10 }) },
                2, 2, // size
                1000, // building cost
                true, // may be destroyed
                15, // workers
                0 // residents
            },
            /* Type::Windmill */ {
                "building_name_windmill",
                &ui_icon::windmill,
                { "res/buildings/windmill.glb", Renderer::model_attribs },
                "blades", 1.0,
                { RelCollider({ -3, -0.5, -3 }, { 6, 1, 6 }) },
                2, 2, // size
                1000, // building cost
                true, // may be destroyed
                5, // workers
                0 // residents
            },
            /* Type::Factory */ {
                "building_name_factory",
                &ui_icon::factory,
                { "res/buildings/factory.glb", Renderer::model_attribs },
                std::nullopt, 0.0,
                { RelCollider({ -5, -0.5, -2.5 }, { 10, 1, 5 }) },
                2, 1, // size
                1000, // building cost
                true, // may be destroyed
                25, // workers
                0 // residents
            },
            /* Type::House */ {
                "building_name_house",
                &ui_icon::house,
                { "res/buildings/house.glb", Renderer::model_attribs },
                "door", 0.0, // speed = 0 -> will always be the first frame
                { RelCollider({ -2.5, -0.5, -1.25 }, { 5, 1, 2.5 }) },
                1, 1, // size
                500, // building cost
                true, // may be destroyed
                0, // workers
                5 // residents
            },
            /* Type::Stable */ {
                "building_name_stable",
                &ui_icon::stable,
                { "res/buildings/stable.glb", Renderer::model_attribs },
                std::nullopt, 0.0,
                { 
                    // building
                    RelCollider({ -7.500, -0.5, -7.500 }, { 15.00, 1, 5.00 }),
                    RelCollider({ -5.500, -0.5, -2.500 }, {  6.00, 1, 3.00 }),
                    RelCollider({  2.500, -0.5, -2.500 }, {  5.00, 1, 5.00 }),
                    // fences
                    RelCollider({ -7.125, -0.5, -2.125 }, {  0.25, 1, 9.25 }),
                    RelCollider({ -7.125, -0.5,  6.875 }, { 12.25, 1, 0.25 }),
                    // trough
                    RelCollider({ -6.500, -0.5,  3.500 }, {  1.00, 1, 3.00 })
                },
                3, 3, // size
                2000, // building cost
                true, // may be destroyed
                20, // workers
                0 // residents
            },
            /* Type::Plaza */ {
                "building_name_plaza",
                &ui_icon::plaza,
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
                3, 3, // size
                0, // building cost (can't be built nor destroyed)
                false, // may not be destroyed
                0, // workers
                0 // residents
            },
            /* Type::Mansion */ {
                "building_name_mansion",
                &ui_icon::mansion,
                { "res/buildings/mansion.glb", Renderer::model_attribs },
                std::nullopt, 0.0,
                { 
                    // colliders
                },
                3, 2, // size
                0, // building cost (can't be built nor destroyed)
                false, // may not be destroyed
                0, // workers
                0 // residents
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
            Stable = 5,
            Plaza = 6,
            Mansion = 7
        };

        Type type;
        u8 x, z; // in tiles relative to chunk origin
        std::optional<ComplexId> complex;


        const TypeInfo& get_type_info() const {
            return Building::types.at((size_t) this->type);
        }

    };

}