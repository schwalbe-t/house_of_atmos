
#pragma once

#include "../renderer.hpp"
#include "../ui_const.hpp"

namespace houseofatmos::world {

    using namespace houseofatmos;

    struct Bridge {

        struct TypeInfo {
            std::string local_name;
            const ui::Background* icon;
            engine::Model::LoadArgs model;
            // in-game units, relative to center
            std::vector<RelCollider> horizontal_colliders;
            std::vector<RelCollider> vertical_colliders;
            i64 min_height;
            i64 max_height;
            u64 cost_per_tile;
        };

        static inline const std::vector<TypeInfo> types = {
            /* Type::Wooden */ {
                "bridge_name_wooden",
                &ui_icon::wooden_bridge,
                { 
                    "res/bridges/wooden_bridge.glb", Renderer::model_attribs,
                    engine::FaceCulling::Enabled
                },
                {
                    RelCollider({ -2.5, -3.0, -2.5 }, { 5.0, 2.5, 5.0 }),
                    RelCollider({ -2.5 - 0.125, 0, -2.5 - 0.125 }, { 5.0 + 0.25, 1, 0.25 }),
                    RelCollider({ -2.5 - 0.125, 0,  2.5 - 0.125 }, { 5.0 + 0.25, 1, 0.25 }),

                    RelCollider({ -2.5 - 0.125, -12,  -2.5 - 0.125 }, { 0.25, 12, 0.25 }),
                    RelCollider({ -2.5 - 0.125, -12,   2.5 - 0.125 }, { 0.25, 12, 0.25 }),
                    RelCollider({  2.5 - 0.125, -12,  -2.5 - 0.125 }, { 0.25, 12, 0.25 }),
                    RelCollider({  2.5 - 0.125, -12,   2.5 - 0.125 }, { 0.25, 12, 0.25 })
                },
                {
                    RelCollider({ -2.5, -3.0, -2.5 }, { 5.0, 2.5, 5.0 }),
                    RelCollider({ -2.5 - 0.125, 0, -2.5 - 0.125 }, { 0.25, 1, 5.0 + 0.25 }),
                    RelCollider({  2.5 - 0.125, 0, -2.5 - 0.125 }, { 0.25, 1, 5.0 + 0.25 }),

                    RelCollider({ -2.5 - 0.125, 0,  -2.5 - 0.125 }, { 0.25, 1, 0.25 }),
                    RelCollider({ -2.5 - 0.125, 0,   2.5 - 0.125 }, { 0.25, 1, 0.25 }),
                    RelCollider({  2.5 - 0.125, 0,  -2.5 - 0.125 }, { 0.25, 1, 0.25 }),
                    RelCollider({  2.5 - 0.125, 0,   2.5 - 0.125 }, { 0.25, 1, 0.25 })
                },
                // min and max height (distance of terrain / water to bridge floor)
                3, 12, 
                1000 // cost of bridge per tile
            },
            /* Type::Stone */ {
                "bridge_name_stone",
                &ui_icon::stone_bridge,
                { 
                    "res/bridges/stone_bridge.glb", Renderer::model_attribs, 
                    engine::FaceCulling::Enabled 
                },
                {
                    RelCollider({ -2.5, -1.0, -2.5 }, { 5.0, 0.5, 5.0 }),
                    RelCollider({ -2.5, 0, -2.5 }, { 5.0, 1, 0.5 }),
                    RelCollider({ -2.5, 0,  2.0 }, { 5.0, 1, 0.5 }),

                    RelCollider({ -2.5, -18, -2.5 }, { 0.5, 17, 5.0 }),
                    RelCollider({  2.0, -18, -2.5 }, { 0.5, 17, 5.0 })
                },
                {
                    RelCollider({ -2.5, -1.0, -2.5 }, { 5.0, 0.5, 5.0 }),
                    RelCollider({ -2.5, 0, -2.5 }, { 0.5, 1, 5.0 }),
                    RelCollider({  2.0, 0, -2.5 }, { 0.5, 1, 5.0 }),

                    RelCollider({ -2.5, -18, -2.5 }, { 5.0, 17, 0.5 }),
                    RelCollider({ -2.5, -18,  2.0 }, { 5.0, 17, 0.5 })
                },
                // min and max height (distance of terrain / water to bridge floor)
                2, 18,
                2000 // cost of bridge per tile
            },
            /* Type::Metal */ {
                "bridge_name_metal",
                &ui_icon::metal_bridge,
                { 
                    "res/bridges/metal_bridge.glb", Renderer::model_attribs,
                    engine::FaceCulling::Disabled
                },
                {
                    RelCollider({ -2.5, -1.0, -2.5 }, { 5.0, 0.5, 5.0 }),
                    RelCollider({ -2.5 - 0.25, 0, -2.5 - 0.25 }, { 5.0 + 0.5, 1, 0.5 }),
                    RelCollider({ -2.5 - 0.25, 0,  2.5 - 0.25 }, { 5.0 + 0.5, 1, 0.5 }),
                },
                {
                    RelCollider({ -2.5, -1.0, -2.5 }, { 5.0, 0.5, 5.0 }),
                    RelCollider({ -2.5 - 0.25, 0, -2.5 - 0.25 }, { 0.5, 1, 5.0 + 0.5 }),
                    RelCollider({  2.5 - 0.25, 0, -2.5 - 0.25 }, { 0.5, 1, 5.0 + 0.5 }),
                },
                // min and max height (distance of terrain / water to bridge floor)
                1, 1024,
                3000 // cost of bridge per tile
            }
        };

        static void load_models(engine::Scene& scene) {
            for(const TypeInfo& type: Bridge::types) {
                scene.load(engine::Model::Loader(type.model));
            }
        }


        enum Type {
            Wooden = 0,
            Stone = 1,
            Metal = 2
        };

        Type type;
        u64 start_x, start_z; // in tiles relative to world origin
        u64 end_x, end_z; // in tiles relative to world origin
        // note: start_x and start_z must always be smaller than end_x and end_z
        i16 floor_y; // elevation of the bridge floor

        const TypeInfo& get_type_info() const {
            return Bridge::types.at((size_t) this->type);
        }

        bool is_vertical() const {
            return this->start_x == this->end_x;
        }

        void report_malformed() const {
            if(this->start_x <= this->end_x) { return; }
            if(this->start_z <= this->end_z) { return; }
            engine::error("Instance of 'Bridge' is malformed!");
        }

        u64 length() const {
            this->report_malformed();
            return 1
                + (this->end_x - this->start_x) 
                + (this->end_z - this->start_z);
        }

        const std::vector<RelCollider>& colliders() const {
            if(this->is_vertical()) { 
                return this->get_type_info().vertical_colliders; 
            }
            return this->get_type_info().horizontal_colliders;
        }

        std::vector<Mat<4>> get_instances(f64 units_per_tile) const {
            this->report_malformed();
            std::vector<Mat<4>> instances;
            instances.reserve(this->length());
            f64 angle = this->is_vertical()
                ? /*  0 degs */ 0         
                : /* 90 degs */ pi / 2.0;
            u64 x = this->start_x;
            u64 z = this->start_z;
            for(;;) {
                Vec<3> offset = Vec<3>(x, 0, z)
                    * units_per_tile
                    + Vec<3>(2.5, this->floor_y, 2.5);
                Mat<4> transform = Mat<4>::translate(offset)
                    * Mat<4>::rotate_y(angle);
                instances.push_back(transform);
                if(x < this->end_x) { x += 1; }
                else if(z < this->end_z) { z += 1; }
                else { break; }
            }
            return instances;
        }

    };

}