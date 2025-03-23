
#include "foliage.hpp"

namespace houseofatmos::world {

    static std::vector<Foliage::TypeInfo> type_infos = {
        /* Grass */ {
            { 
                "res/foliage/grass.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            },
            RelCollider::none(),
            10,
            [](f64 nmap) { (void) nmap; return 1.0; } // chance is always 100%
        },
        /* Birch */ {
            {
                "res/foliage/birch.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            },
            RelCollider({ -5.0/16, -0.5, -5.0/16 }, { 10.0/16, 1, 10.0/16 }),
            1,
            [](f64 nmap) {
                return nmap < 0.4? 0.05  // for 40% of area =>  5% chance
                     : nmap < 0.6? 0.50  // for 20% of area => 50% chance
                     :             0.80; // for 40% of area => 80% chance
            }
        },
        /* Rock */ {
            { 
                "res/foliage/rocks.glb", Renderer::model_attribs,
                engine::FaceCulling::Enabled
            },
            RelCollider({ -0.5, -0.5, -0.5 }, { 1, 1, 1 }),
            1,
            [](f64 nmap) { (void) nmap; return 0.1; } // chance is always 10%
        },
        /* BirchSapling */ {
            {
                "res/foliage/birch_sapling.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            },
            RelCollider({ -1/16, -0.5, -1/16 }, { 2/16, 1, 2/16 }),
            1,
            [](f64 nmap) {
                return nmap < 0.4? 0.01  // for 40% of area =>  1% chance
                     : nmap < 0.6? 0.10  // for 20% of area => 10% chance
                     :             0.25; // for 40% of area => 25% chance
            }
        },
        /* Pine */ {
            {
                "res/foliage/pine.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            },
            RelCollider({ -5.0/16, -0.5, -5.0/16 }, { 10.0/16, 1, 10.0/16 }),
            1,
            [](f64 nmap) {
                return nmap < 0.4? 0.025  // for 40% of area =>  2.5% chance
                     : nmap < 0.6? 0.250  // for 20% of area => 25.0% chance
                     :             0.400; // for 40% of area => 40.0% chance
            }
        },
        /* PineSapling */ {
            {
                "res/foliage/pine_sapling.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            },
            RelCollider({ -1/16, -0.5, -1/16 }, { 2/16, 1, 2/16 }),
            1,
            [](f64 nmap) {
                return nmap < 0.4? 0.005  // for 40% of area =>  0.5% chance
                     : nmap < 0.6? 0.050  // for 20% of area =>  5.0% chance
                     :             0.125; // for 40% of area => 12.5% chance
            }
        },
    };

    const std::vector<Foliage::TypeInfo>& Foliage::types() {
        return type_infos;
    }

}