
#pragma once

#include "rendering.hpp"
#include "animation.hpp"

namespace houseofatmos::engine {

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


        private:
        std::vector<Texture> textures;
        std::unordered_map<std::string, std::pair<std::unique_ptr<Mesh>, size_t>> meshes;
        std::vector<Mat<4>> inverse_bind;
        Animation::Skeleton bones;
        std::unordered_map<std::string, Animation> animations;

        Model();

        public:
        static Model from_resource(const LoadArgs& args);
        Model(const Model& other) = delete;
        Model(Model&& other) = default;
        Model& operator=(const Model& other) = delete;
        Model& operator=(Model&& other) = default;
        ~Model() = default;


        std::tuple<Mesh&, const Texture&> primitive(
            const std::string& primitive_name
        );
        const Animation& animation(const std::string& animation_name) const {
            return this->animations.at(animation_name);
        }
        const Animation::Skeleton& skeleton() const { return this->bones; }
        
        void render_all(
            Shader& shader, const Texture& dest,
            std::string_view texture_uniform,
            bool depth_test = true
        );

    };

};