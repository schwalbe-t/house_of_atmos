
#include "building.hpp"
#include "../particle_const.hpp"

namespace houseofatmos::world {

    static const ParticleSpawner::Type factory_smoke = {
        3.0, // attempt period in seconds
        1.0, // spawn chance on every attempt
        // spawn function
        [](ParticleSpawner& spawner, ParticleManager& particles) { 
            particles.add(particle::random_smoke(spawner.rng)
                ->at(spawner.position + Vec<3>(-3.000, 10.500, -1.312))
            );
            particles.add(particle::random_smoke(spawner.rng)
                ->at(spawner.position + Vec<3>(-1.000, 10.500, -1.312))
            );
        }
    };

    static const ParticleSpawner::Type house_smoke = {
        7.5, // attempt period in seconds
        1.0, // spawn chance on every attempt
        // spawn function
        [](ParticleSpawner& spawner, ParticleManager& particles) { 
            particles.add(particle::random_smoke(spawner.rng)
                ->at(spawner.position + Vec<3>(-1.742, 3.769, -0.433))
            );
        }
    };

    static const ParticleSpawner::Type commissary_works_smoke = {
        5.0, // attempt period in seconds
        1.0, // spawn chance on every attempt
        // spawn function
        [](ParticleSpawner& spawner, ParticleManager& particles) { 
            particles.add(particle::random_smoke(spawner.rng)
                ->at(spawner.position + Vec<3>(2.5, 4.0, -1.5))
            );
            particles.add(particle::random_smoke(spawner.rng)
                ->at(spawner.position + Vec<3>(4.0, 4.0, -1.5))
            );
        }
    };

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
            10, // storage capacity
            Building::TypeInfo::allow_destruction,
            5, // workers
            0, // residents
            std::nullopt // no particles
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
            10, // storage capacity
            Building::TypeInfo::allow_destruction,
            15, // workers
            0, // residents
            std::nullopt // no particles
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
            10, // storage capacity
            Building::TypeInfo::allow_destruction,
            5, // workers
            0, // residents
            std::nullopt // no particles
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
            10, // storage capacity
            Building::TypeInfo::allow_destruction,
            25, // workers
            0, // residents
            [](Vec<3> p, StatefulRNG& r) { return factory_smoke.at(p, r); }
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
            0, // storage capacity
            Building::TypeInfo::allow_destruction,
            0, // workers
            5, // residents
            [](Vec<3> p, StatefulRNG& r) {
                bool emits_smoke = r.next_f64() < 0.10;
                Vec<3> position = emits_smoke? p : Vec<3>(1.0, 1.0, 1.0) / 0.0;
                return house_smoke.at(position, r);
            }
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
            0, // storage capacity
            Building::TypeInfo::allow_destruction,
            20, // workers
            0, // residents
            std::nullopt // no particles
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
            500, // storage capacity
            Building::TypeInfo::indestructible,
            0, // workers
            0, // residents
            std::nullopt // no particles
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
            0, // storage capacity
            Building::TypeInfo::indestructible,
            0, // workers
            0, // residents
            std::nullopt // no particles
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
            10, // storage capacity
            Building::TypeInfo::allow_destruction,
            5, // workers
            0, // residents
            std::nullopt // no particles
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
            10, // storage capacity
            Building::TypeInfo::allow_destruction,
            5, // workers
            0, // residents
            std::nullopt // no particles
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
            0, // storage capacity
            Building::TypeInfo::allow_destruction,
            20, // workers
            0, // residents
            std::nullopt // no particles
        },
        /* Type::ShipYard */ {
            "building_name_ship_yard",
            &ui_icon::ship_yard,
            Building::TypeInfo::remove_terrain,
            { 
                "res/buildings/ship_yard.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled    
            },
            std::nullopt, 0.0,
            {
                RelCollider({ -7.5, -0.5, -7.5 }, { 10.0, 1, 15.0 }),
                RelCollider({  2.5, -0.5, -7.5 }, {  5.0, 1, 10.0 })
            },
            3, 3, // size
            std::nullopt, // no interior
            5000, // building cost
            0, // storage capacity
            Building::TypeInfo::allow_destruction,
            30, // workers
            0, // residents
            std::nullopt // no particles
        },
        /* Type::Storage */ {
            "building_name_storage",
            &ui_icon::storage,
            Building::TypeInfo::keep_terrain,
            { 
                "res/buildings/storage.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            },
            std::nullopt, 0.0,
            { RelCollider({ -2.5, -0.5, -5 }, { 5, 1, 10 }) },
            1, 2, // size
            std::nullopt, // no interior
            500, // building cost
            500, // storage capacity
            Building::TypeInfo::allow_destruction,
            0, // workers
            0, // residents
            std::nullopt // no particles
        },
        /* Type::CommissaryWorks */ {
            "building_name_commissary_works",
            &ui_icon::commissary_works,
            Building::TypeInfo::remove_terrain,
            { 
                "res/buildings/commissary_works.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            },
            std::nullopt, 0.0,
            { 
                RelCollider({ -5.00, -0.5, -2.5 }, { 3.0, 1, 5.0 }),
                RelCollider({ -2.00, -0.5, -2.5 }, { 7.0, 1, 2.5 }),
                RelCollider({  1.45, -0.5,  2.2 }, { 0.1, 1, 0.1 }),
                RelCollider({  4.70, -0.5,  2.2 }, { 0.1, 1, 0.1 })
            },
            2, 1, // size
            std::nullopt, // no interior
            1000, // building cost
            10, // storage capacity
            Building::TypeInfo::allow_destruction,
            25, // workers
            0, // residents
            [](Vec<3> p, StatefulRNG& r) { 
                return commissary_works_smoke.at(p, r); 
            }
        },
        /* Type::ManufacturingWorks */ {
            "building_name_manufacturing_works",
            &ui_icon::manufacturing_works,
            Building::TypeInfo::keep_terrain,
            { 
                "res/buildings/manufacturing_works.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            },
            std::nullopt, 0.0,
            { RelCollider({ -5.0, -0.5, -7.5 }, { 10, 1, 15 }) },
            2, 3, // size
            std::nullopt, // no interior
            2000, // building cost
            10, // storage capacity
            Building::TypeInfo::allow_destruction,
            25, // workers
            0, // residents
            std::nullopt // no particles
        },
        /* Type::ClothWorks */ {
            "building_name_cloth_works",
            &ui_icon::cloth_works,
            Building::TypeInfo::keep_terrain,
            { 
                "res/buildings/cloth_works.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            },
            std::nullopt, 0.0,
            { 
                RelCollider({ -7.50, -0.5, -2.5 }, { 5.0, 1, 5.0 }),
                RelCollider({  2.50, -0.5, -2.5 }, { 5.0, 1, 5.0 })
            },
            3, 1, // size
            std::nullopt, // no interior
            1500, // building cost
            10, // storage capacity
            Building::TypeInfo::allow_destruction,
            15, // workers
            0, // residents
            std::nullopt // no particles
        },
    };

    const std::vector<Building::TypeInfo>& Building::types() {
        return type_infos;
    }

}