
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
        f64 fov = pi / 6; // 30 degrees
        f64 near_plane = 0.1;
        f64 far_plane = 1000;
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

        static const inline size_t max_inst_c = 128;

        private:
        engine::Texture target = engine::Texture(100, 100);

        public:
        Camera camera;
        u64 resolution = 360;
        Vec<3> light_direction = { -2, -5, -1 };
        f64 ambient_light = 0.8; 
        engine::Shader* shader = nullptr;

        static void load_shaders(engine::Scene& scene) {
            scene.load(engine::Shader::Loader(Renderer::shader_args));
        }

        Mat<4> compute_view_matrix() const;
        Mat<4> compute_proj_matrix() const;
        Mat<4> compute_view_proj() const;
        void configure(const engine::Window& window, engine::Scene& scene);

        Vec<2> world_to_ndc(const Vec<3>& pos) const;

        void render(
            engine::Mesh& mesh, 
            const engine::Texture& texture,
            const Mat<4>& local_transform = Mat<4>(),
            std::span<const Mat<4>> model_transforms
                = std::array<Mat<4>, 1> { Mat<4>() },
            bool wireframe = false,
            bool depth_test = true
        ) const;
        void render(
            engine::Model& model,
            std::span<const Mat<4>> model_transforms,
            bool wireframe = false,
            const engine::Texture* override_texture = nullptr,
            bool depth_test = true
        ) const;
        void render(
            engine::Model& model,
            std::span<const Mat<4>> model_transforms,
            const engine::Animation& animation,
            f64 timestamp,
            bool wireframe = false,
            const engine::Texture* override_texture = nullptr,
            bool depth_test = true
        ) const;

        const engine::Texture& output() const { return this->target; }

    };

}