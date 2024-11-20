
#pragma once

#include "rendering.hpp"
#include "animation.hpp"
#include <array>
#include <unordered_map>

namespace houseofatmos::engine::resources {
    
    using namespace houseofatmos::engine::math;
    namespace rendering = houseofatmos::engine::rendering;
    namespace animation = houseofatmos::engine::animation;


    std::string read_string(const char* file);

    struct ModelVertex {
        Vec<3> pos;
        Vec<2> uv;
        Vec<3> normal;
    };
    rendering::Mesh<ModelVertex> read_obj_model(const char* file);

    rendering::Surface read_texture(const char* file);

    #define RIGGED_MESH_MAX_VERTEX_JOINTS 4
    struct RiggedModelVertex {
        Vec<3> pos;
        Vec<2> uv;
        Vec<3> normal;
        Vec<RIGGED_MESH_MAX_VERTEX_JOINTS> weights;
        std::array<uint8_t, RIGGED_MESH_MAX_VERTEX_JOINTS> joints;
    };
    struct RiggedModelMesh {
        rendering::Mesh<RiggedModelVertex> mesh;
        Mat<4> local_transform;
        uint8_t texture;
    };
    struct RiggedModelBone {
        Mat<4> inverse_bind;
        Mat<4> anim_transform;
        std::vector<uint8_t> children;
        bool has_parent;
    };
    struct RiggedModel {
        std::vector<RiggedModelMesh> meshes;
        std::vector<rendering::Surface> textures;
        std::vector<RiggedModelBone> bones;
        std::unordered_map<std::string, animation::Animation> animations;

        template<typename S>
        void draw(rendering::Surface& surface, S& shader) {
            for(size_t mesh_i = 0; mesh_i < this->meshes.size(); mesh_i += 1) {
                RiggedModelMesh& mesh = this->meshes[mesh_i];
                shader.local = mesh.local_transform;
                shader.texture = &this->textures[mesh.texture];
                surface.draw_mesh(mesh.mesh, shader);
                shader.texture = NULL;
            }
        }
    };
    RiggedModel read_gltf_model(const char* file);

}