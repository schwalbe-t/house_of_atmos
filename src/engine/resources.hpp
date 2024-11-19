
#pragma once

#include "rendering.hpp"


namespace houseofatmos::engine::resources {
    
    using namespace houseofatmos::engine::math;
    namespace rendering = houseofatmos::engine::rendering;


    std::string read_string(const char* file);

    struct ModelVertex {
        Vec<3> pos;
        Vec<2> uv;
        Vec<3> normal;
    };
    rendering::Mesh<ModelVertex> read_obj_model(const char* file);

    rendering::Surface read_texture(const char* file);

    #define RIGGED_MESH_MAX_VERTEX_BONES 4
    struct RiggedModelVertex {
        Vec<3> pos;
        Vec<2> uv;
        Vec<3> normal;
        float bone_weights[RIGGED_MESH_MAX_VERTEX_BONES];
        uint8_t bone_indices[RIGGED_MESH_MAX_VERTEX_BONES];
    };
    struct RiggedModelMesh {
        rendering::Mesh<RiggedModelVertex> mesh;
        uint8_t texture;
    };
    struct RiggedModelBone {
        Mat<4> inverse_bind;
        Mat<4> current_transformation;
        std::vector<uint8_t> children;
        bool has_parent;
    };
    struct RiggedModel {
        std::vector<RiggedModelMesh> meshes;
        std::vector<rendering::Surface> textures;
        std::vector<RiggedModelBone> bones;
    };
    RiggedModel read_gltf_model(const char* file);

}