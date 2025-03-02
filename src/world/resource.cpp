
#include "resource.hpp"

namespace houseofatmos::world {

    static const engine::Model::LoadArgs ore_model = {
        "res/resources/ore_resource.glb", Renderer::model_attribs,
        engine::FaceCulling::Enabled
    };

    static std::vector<Resource::TypeInfo> type_infos = {
        // The 'true' in each texture loader is there to flip the loaded
        // texture vertically - this is needed since GLTF uses them flipped
        /* Coal */ {
            "item_name_coal",
            ore_model,
            (engine::Texture::LoadArgs) { 
                "res/resources/coal_ore.png", 
                engine::Texture::vertical_mirror
            },
            1.0 / 1500.0 // spawn chance, on avg. approx. 43.6 in 256x256 
        },
        /* CrudeOil */ {
            "item_name_crude_oil",
            engine::Model::LoadArgs(
                "res/resources/crude_oil_resource.glb", 
                Renderer::model_attribs,
                engine::FaceCulling::Disabled
            ),
            (engine::Texture::LoadArgs) { 
                "res/resources/crude_oil.png", 
                engine::Texture::vertical_mirror
            },
            1.0 / 1500.0 // spawn chance, on avg. approx. 43.6 in 256x256 
        },
        /* Salt */ {
            "item_name_salt",
            ore_model,
            (engine::Texture::LoadArgs) { 
                "res/resources/salt.png", 
                engine::Texture::vertical_mirror
            },
            1.0 / 1500.0 // spawn chance, on avg. approx. 43.6 in 256x256 
        },
        /* IronOre */ {
            "item_name_iron_ore",
            ore_model,
            (engine::Texture::LoadArgs) { 
                "res/resources/iron_ore.png", 
                engine::Texture::vertical_mirror
            },
            1.0 / 1500.0 // spawn chance, on avg. approx. 43.6 in 256x256 
        },
        /* CopperOre */ {
            "item_name_copper_ore",
            ore_model,
            (engine::Texture::LoadArgs) { 
                "res/resources/copper_ore.png", 
                engine::Texture::vertical_mirror
            },
            1.0 / 1500.0 // spawn chance, on avg. approx. 43.6 in 256x256 
        },
        /* ZincOre */ {
            "item_name_zinc_ore",
            ore_model,
            (engine::Texture::LoadArgs) { 
                "res/resources/zinc_ore.png", 
                engine::Texture::vertical_mirror
            },
            1.0 / 1500.0 // spawn chance, on avg. approx. 43.6 in 256x256 
        }
    };

    const std::vector<Resource::TypeInfo>& Resource::types() {
        return type_infos;
    }    

}