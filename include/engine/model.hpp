
#pragma once

#include "rendering.hpp"
#include "animation.hpp"
#include <tuple>
#include <optional>

namespace houseofatmos::engine {

    using namespace math;


    struct Model {

        enum Attrib {
            Position,
            UvMapping,
            Normal,
            Weights,
            Joints
        };

        static std::string gltf_attrib_name(Model::Attrib attrib);

        struct LoadArgs {
            using ResourceType = Model;

            std::string_view path;
            std::vector<std::pair<Attrib, Mesh::Attrib>> vertex_attributes;
            FaceCulling face_culling;
            LoadArgs(
                std::string_view path, 
                std::vector<std::pair<Attrib, Mesh::Attrib>> vertex_attribs, 
                FaceCulling face_culling = FaceCulling::Disabled
            ): path(path), vertex_attributes(std::move(vertex_attribs)), 
                face_culling(face_culling) {}

            std::string pretty_identifier() const { 
                std::string r = "Model[";
                for(const auto& [model_attrib, mesh_attrib]: vertex_attributes) {
                    r += gltf_attrib_name(model_attrib);
                    r += " = " + mesh_attrib.display();
                    r += ", ";
                }
                r += ", " + std::string(face_culling == FaceCulling::Enabled
                    ? "<culling allowed>" : "<no culling>"
                );
                return r + "]@'" + std::string(path) + "'";
            }
            std::string identifier() const {
                return this->pretty_identifier(); 
            }
        };


        struct Primitive {
            Mesh geometry;
            Mat<4> local_transform;
        };

        private:
        std::vector<Texture> textures;
        std::vector<Primitive> primitives;
        std::vector<Animation::Skeleton> skeletons;
        std::unordered_map<std::string, std::tuple<size_t, size_t, std::optional<size_t>>> meshes;
        std::unordered_map<std::string, Animation> animations;

        Model();

        public:
        FaceCulling face_culling = FaceCulling::Enabled;

        static Model from_resource(const LoadArgs& args);
        Model(const Model& other) = delete;
        Model(Model&& other) = default;
        Model& operator=(const Model& other) = delete;
        Model& operator=(Model&& other) = default;
        ~Model() = default;


        std::tuple<Primitive&, const Texture&, const Animation::Skeleton*> mesh(
            const std::string& primitive_name
        );

        const Animation& animation(const std::string& animation_name) const {
            auto animation = this->animations.find(animation_name);
            if(animation != this->animations.end()) {
                return animation->second;
            }
            error(
                "Model does not contain an animation called '" 
                + animation_name + "'!"
            );
        }
        
        void render_all(
            Shader& shader, RenderTarget dest,
            std::optional<std::string_view> local_transform_uniform = std::nullopt,
            std::optional<std::string_view> texture_uniform = std::nullopt,
            std::optional<std::string_view> joint_transform_uniform = std::nullopt,
            size_t count = 1,
            FaceCulling face_culling = FaceCulling::Enabled,
            Rendering rendering = Rendering::Surfaces, 
            DepthTesting depth_testing = DepthTesting::Enabled
        );

        void render_all_animated(
            Shader& shader, RenderTarget dest,
            const Animation& animation, f64 timestamp,
            std::string_view joint_transform_uniform,
            std::optional<std::string_view> local_transform_uniform = std::nullopt,
            std::optional<std::string_view> texture_uniform = std::nullopt,
            size_t count = 1,
            FaceCulling face_culling = FaceCulling::Enabled,
            Rendering rendering = Rendering::Surfaces, 
            DepthTesting depth_testing = DepthTesting::Enabled
        );

    };

};