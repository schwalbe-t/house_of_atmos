
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
        /* Tree */ {
            {
                "res/foliage/tree.glb", Renderer::model_attribs,
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
        /* Rocks */ {
            { 
                "res/foliage/rocks.glb", Renderer::model_attribs,
                engine::FaceCulling::Enabled
            },
            RelCollider({ -0.5, -0.5, -0.5 }, { 1, 1, 1 }),
            1,
            [](f64 nmap) { (void) nmap; return 0.1; } // chance is always 10%
        },
    };

    const std::vector<Foliage::TypeInfo>& Foliage::types() {
        return type_infos;
    }

}