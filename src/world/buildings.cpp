
#include "buildings.hpp"

namespace houseofatmos::world {

    static std::vector<Building::TypeInfo> type_infos = {
        /* Type::Farmland */ {
            "building_name_farmland",
            &ui_icon::farmland,
            Building::TypeInfo::keep_terrain,
            { 
                "res/buildings/farmland.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            },
            std::nullopt, 0.0,
            {
                // corner fence
                RelCollider({ -4.625, -0.5, -4.625 }, { 0.25, 1, 6.25 }),
                RelCollider({ -4.625, -0.5, -4.625 }, { 3.25, 1, 0.25 }),
                // long fence
                RelCollider({  4.425, -0.5, -4.625 }, { 0.25, 1, 9.25 })
            },
            2, 2, // size
            std::nullopt, // no interior
            500, // building cost
            Building::TypeInfo::allow_destruction,
            5, // workers
            0 // residents
        },
        /* Type::Mineshaft */ {
            "building_name_mineshaft",
            &ui_icon::mineshaft,
            Building::TypeInfo::remove_terrain,
            { 
                "res/buildings/mineshaft.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            },
            std::nullopt, 0.0,
            { 
                RelCollider({ -3, -0.5, -3 }, { 6, 1, 6 }),
                RelCollider({  3, -0.5, -1 }, { 2, 1, 2 }) 
            },
            2, 2, // size
            std::nullopt, // no interior
            1000, // building cost
            Building::TypeInfo::allow_destruction,
            15, // workers
            0 // residents
        },
        /* Type::Windmill */ {
            "building_name_windmill",
            &ui_icon::windmill,
            Building::TypeInfo::keep_terrain,
            { 
                "res/buildings/windmill.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            },
            "blades", 1.0,
            { RelCollider({ -3, -0.5, -3 }, { 6, 1, 6 }) },
            2, 2, // size
            std::nullopt, // no interior
            1000, // building cost
            Building::TypeInfo::allow_destruction,
            5, // workers
            0 // residents
        },
        /* Type::Factory */ {
            "building_name_factory",
            &ui_icon::factory,
            Building::TypeInfo::keep_terrain,
            { 
                "res/buildings/factory.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            },
            std::nullopt, 0.0,
            { RelCollider({ -5, -0.5, -2.5 }, { 10, 1, 5 }) },
            2, 1, // size
            std::nullopt, // no interior
            1000, // building cost
            Building::TypeInfo::allow_destruction,
            25, // workers
            0 // residents
        },
        /* Type::House */ {
            "building_name_house",
            &ui_icon::house,
            Building::TypeInfo::keep_terrain,
            { 
                "res/buildings/house.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled    
            },
            "door", 0.0, // speed = 0 -> will always be the first frame
            { RelCollider({ -2.5, -0.5, -1.25 }, { 5, 1, 2.5 }) },
            1, 1, // size
            std::nullopt, // no interior
            500, // building cost
            Building::TypeInfo::allow_destruction,
            0, // workers
            5 // residents
        },
        /* Type::Stable */ {
            "building_name_stable",
            &ui_icon::stable,
            Building::TypeInfo::keep_terrain,
            { 
                "res/buildings/stable.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            },
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
            std::nullopt, // no interior
            2000, // building cost
            Building::TypeInfo::allow_destruction,
            20, // workers
            0 // residents
        },
        /* Type::Plaza */ {
            "building_name_plaza",
            &ui_icon::plaza,
            Building::TypeInfo::remove_terrain,
            { 
                "res/buildings/plaza.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled    
            },
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
            std::nullopt, // no interior
            0, // building cost (can't be built nor destroyed)
            Building::TypeInfo::indestructible,
            0, // workers
            0 // residents
        },
        /* Type::Mansion */ {
            "building_name_mansion",
            &ui_icon::mansion,
            Building::TypeInfo::keep_terrain,
            { 
                "res/buildings/mansion.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            },
            std::nullopt, 0.0,
            { 
                RelCollider({ -7.5, -0.5, -5.0 }, { 15.0, 1,  5.0 }),
                RelCollider({ -7.5, -0.5, -5.0 }, {  5.0, 1, 10.0 }),
                RelCollider({  2.5, -0.5, -5.0 }, {  5.0, 1, 10.0 })
            },
            3, 2, // size
            (Building::LinkedInterior) { { 0, 1, 0 }, interior::mansion },
            0, // building cost (can't be built nor destroyed)
            Building::TypeInfo::indestructible,
            0, // workers
            0 // residents
        },
        /* Type::Pasture */ {
            "building_name_pasture",
            &ui_icon::pasture,
            Building::TypeInfo::keep_terrain,
            {
                "res/buildings/pasture.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            },
            std::nullopt, 0.0,
            {
                // corner fence
                RelCollider({ -4.625, -0.5, -4.625 }, { 0.25, 1, 6.25 }),
                RelCollider({ -4.625, -0.5, -4.625 }, { 3.25, 1, 0.25 }),
                // long fence
                RelCollider({  4.425, -0.5, -4.625 }, { 0.25, 1, 9.25 })
            },
            2, 2, // size
            std::nullopt, // no interior
            500, // building cost
            Building::TypeInfo::allow_destruction,
            5, // workers
            0 // residents
        },
        /* Type::TreeFarm */ {
            "building_name_tree_farm",
            &ui_icon::tree_farm,
            Building::TypeInfo::keep_terrain,
            {
                "res/buildings/tree_farm.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            },
            std::nullopt, 0.0,
            {
                // corner fence
                RelCollider({ -4.625, -0.5, -4.625 }, { 0.25, 1, 6.25 }),
                RelCollider({ -4.625, -0.5, -4.625 }, { 3.25, 1, 0.25 }),
                // long fence
                RelCollider({  4.425, -0.5, -4.625 }, { 0.25, 1, 9.25 })
            },
            2, 2, // size
            std::nullopt, // no interior
            500, // building cost
            Building::TypeInfo::allow_destruction,
            5, // workers
            0 // residents
        },
        /* Type::TrainDepot */ {
            "building_name_train_depot",
            &ui_icon::depot,
            Building::TypeInfo::keep_terrain,
            { 
                "res/buildings/depot.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            },
            std::nullopt, 0.0,
            { 
                RelCollider({ -7.5, -0.5, -7.0 }, {  5.0, 1, 10.0 }),
                RelCollider({ -2.5, -0.5, -7.5 }, { 10.0, 1, 15.0 })
            },
            3, 3, // size
            std::nullopt, // no interior
            5000, // building cost
            Building::TypeInfo::allow_destruction,
            20, // workers
            0 // residents
        },
    };

    const std::vector<Building::TypeInfo>& Building::types() {
        return type_infos;
    }

}