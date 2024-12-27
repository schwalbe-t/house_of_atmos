
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

        struct LoadArgs {
            std::string path;
            std::vector<std::pair<Attrib, Mesh::Attrib>> vertex_attributes;

            std::string identifier() const { return path; }
            std::string pretty_identifier() const { 
                return "Model@'" + path + "'"; 
            }
        };

        using Loader = Resource<Model, LoadArgs>;


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
            return this->animations.at(animation_name);
        }
        
        void render_all(
            Shader& shader, const Texture& dest,
            std::optional<std::string_view> local_transform_uniform = std::nullopt,
            std::optional<std::string_view> texture_uniform = std::nullopt,
            std::optional<std::string_view> joint_transform_uniform = std::nullopt,
            size_t count = 1,
            bool depth_test = true
        );

        void render_all_animated(
            Shader& shader, const Texture& dest,
            const Animation& animation, f64 timestamp,
            std::string_view joint_transform_uniform,
            std::optional<std::string_view> local_transform_uniform = std::nullopt,
            std::optional<std::string_view> texture_uniform = std::nullopt,
            bool depth_test = true
        );

    };

};