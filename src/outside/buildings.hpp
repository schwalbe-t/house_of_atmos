
#pragma once

#include "../renderer.hpp"

namespace houseofatmos::outside {

    using namespace houseofatmos;

    struct Building {

        struct Type {
            engine::Model::LoadArgs model;
            u64 width;
            u64 height;
        };

        const Type* type;
        u64 x, y;

    };

}

namespace houseofatmos::outside::buildings {

    inline const Building::Type farmland = {
        { "res/buildings/farmland.gltf", Renderer::model_attribs },
        10, 10
    };


    inline void load_models(engine::Scene& scene) {
        scene.load(engine::Model::Loader(farmland.model));
    }

}