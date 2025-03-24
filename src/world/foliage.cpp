
#include "foliage.hpp"

namespace houseofatmos::world {

    static std::vector<Foliage::TypeInfo> type_infos = {
        /* Grass */ {
            { 
                "res/foliage/grass.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            },
            RelCollider::none(),
            7,
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
                     : nmap < 0.6? 0.25  // for 20% of area => 25% chance
                     :             0.40; // for 40% of area => 40% chance
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
                return nmap < 0.4? 0.005  // for 40% of area =>  0.5% chance
                     : nmap < 0.6? 0.250  // for 20% of area => 25.0% chance
                     :             0.005; // for 40% of area =>  0.5% chance
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
                     : nmap < 0.6? 0.125  // for 20% of area => 12.5% chance
                     :             0.200; // for 40% of area => 20.0% chance
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
                     : nmap < 0.6? 0.250  // for 20% of area => 25.0% chance
                     :             0.005; // for 40% of area =>  0.5% chance
            }
        },
        /* Fern */ {
            {
                "res/foliage/fern.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            },
            RelCollider::none(),
            1,
            [](f64 nmap) { (void) nmap; return 0.25; } // chance is always 25%
        },
        /* BlueFlowers */ {
            {
                "res/foliage/flowers_blue.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            },
            RelCollider::none(),
            1,
            [](f64 nmap) {
                return nmap < 0.4? 0.50  // for 40% of area => 50% chance
                     : nmap < 0.6? 0.15  // for 20% of area => 15% chance
                     :             0.05; // for 40% of area =>  5% chance
            }
        },
        /* RedFlowers */ {
            {
                "res/foliage/flowers_red.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            },
            RelCollider::none(),
            1,
            [](f64 nmap) {
                return nmap < 0.4? 0.50  // for 40% of area => 50% chance
                     : nmap < 0.6? 0.15  // for 20% of area => 15% chance
                     :             0.05; // for 40% of area =>  5% chance
            }
        },
        /* YellowFlowers */ {
            {
                "res/foliage/flowers_yellow.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            },
            RelCollider::none(),
            1,
            [](f64 nmap) {
                return nmap < 0.4? 0.50  // for 40% of area => 50% chance
                     : nmap < 0.6? 0.15  // for 20% of area => 15% chance
                     :             0.05; // for 40% of area =>  5% chance
            }
        },
        /* Mushroom */ {
            {
                "res/foliage/mushroom.glb", Renderer::model_attribs,
                engine::FaceCulling::Enabled
            },
            RelCollider::none(),
            1,
            [](f64 nmap) {
                return nmap < 0.4? 0     // for 40% of area => 0.0% chance
                     : nmap < 0.6? 0.025 // for 20% of area => 2.5% chance
                     :             0.05; // for 40% of area => 5.0% chance
            }
        },
    };

    const std::vector<Foliage::TypeInfo>& Foliage::types() {
        return type_infos;
    }

}