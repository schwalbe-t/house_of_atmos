
#pragma once

#include <engine/window.hpp>
#include <engine/model.hpp>

namespace houseofatmos {

    using namespace houseofatmos;
    using namespace houseofatmos::engine::math;


    struct Camera {
        Vec<3> position = { 0, 0, 0 };
        Vec<3> look_at = { 0, 0, 0 };
        Vec<3> up = { 0, 1, 0 };
        f64 fov = pi / 6; // 30
        f64 near = 0.1;
        f64 far = 1000;
    };


    using ModelAttrib = std::pair<engine::Model::Attrib, engine::Mesh::Attrib>;

    struct Renderer {

        static const inline std::vector<engine::Mesh::Attrib> mesh_attribs = {
            { engine::Mesh::F32, 3 }, 
            { engine::Mesh::F32, 2 }, 
            { engine::Mesh::F32, 3 },
            { engine::Mesh::U8, 4 },
            { engine::Mesh::F32, 4 }
        };
        static const inline std::vector<ModelAttrib> model_attribs = {
            { engine::Model::Position, { engine::Mesh::F32, 3 } }, 
            { engine::Model::UvMapping, { engine::Mesh::F32, 2 } }, 
            { engine::Model::Normal, { engine::Mesh::F32, 3 } },
            { engine::Model::Joints, { engine::Mesh::U8, 4 } },
            { engine::Model::Weights, { engine::Mesh::F32, 4 } }
        };

        static const inline engine::Shader::LoadArgs shader_args = {
            "res/shaders/geometry_vert.glsl", "res/shaders/geometry_frag.glsl"
        };

        private:
        engine::Texture target = engine::Texture(100, 100);
        engine::Shader* shader = nullptr;

        public:
        Camera camera;
        i64 resolution = 360;
        Vec<3> light_direction = { -2, -5, -1 };
        f64 ambient_light = 0.75; 

        static void load_shaders(engine::Scene& scene) {
            scene.load(engine::Shader::Loader(Renderer::shader_args));
        }

        void configure(const engine::Window& window, engine::Scene& scene);

        void render(
            engine::Mesh& mesh, 
            const engine::Texture& texture,
            const Mat<4>& local_model_transform = Mat<4>()
        ) const;
        void render(
            engine::Model& model,
            const Mat<4>& model_transform = Mat<4>()
        ) const;

        const engine::Texture& output() const { return this->target; }

    };

}