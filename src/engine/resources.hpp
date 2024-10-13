
#pragma once

#include "engine.hpp"

using namespace houseofatmos::engine::math;

namespace houseofatmos::engine::resources {

    std::string read_string(const char* file);

    struct ModelVertex {
        Vec<3> pos;
        Vec<2> uv;
        Vec<3> normal;
    };
    engine::rendering::Mesh<ModelVertex> read_model(const char* file);

    engine::rendering::Surface read_texture(const char* file);

}