
#pragma once

#include <engine/window.hpp>
#include <engine/model.hpp>
#include "light.hpp"

namespace houseofatmos {

    using namespace houseofatmos;
    using namespace houseofatmos::engine::math;


    struct Camera {
        Vec<3> position = { 0, 0, 0 };
        Vec<3> look_at = { 0, 0, 0 };
        Vec<3> up = { 0, 1, 0 };
        f64 fov = pi / 6; // 30 degrees
        f64 near_plane = 0.1;
        f64 far_plane = 1000.0;
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

        static const inline engine::Shader::LoadArgs shadow_shader_args = {
            "res/shaders/geometry_vert.glsl", "res/shaders/shadow_frag.glsl"
        };

        static const inline engine::Shader::LoadArgs geometry_shader_args = {
            "res/shaders/geometry_vert.glsl", "res/shaders/geometry_frag.glsl"
        };

        static const inline size_t max_inst_c = 128;
        static const inline size_t max_light_c = 16;

        private:
        engine::Texture target = engine::Texture(100, 100);
        engine::TextureArray shadow_maps
            = engine::TextureArray(std::span<const engine::Texture>()); 
        bool rendering_shadow_maps = false;

        public:
        Camera camera;
        std::vector<DirectionalLight> lights;
        u64 resolution = 360;
        u64 shadow_map_resolution = 1024;
        f64 fog_start_dist = INFINITY;
        f64 fog_gradiant_range = 0.0;
        Vec<3> fog_origin = Vec<3>(0.0, 0.0, 0.0);
        Vec<3> fog_dist_scale = Vec<3>(1.0, 1.0, 1.0);
        Vec<4> fog_color = Vec<4>(1.0, 1.0, 1.0, 1.0);
        f64 shadow_bias = 0.0;
        engine::Shader* shadow_shader = nullptr;
        engine::Shader* geometry_shader = nullptr;

        static void load_shaders(engine::Scene& scene) {
            scene.load(engine::Shader::Loader(Renderer::shadow_shader_args));
            scene.load(engine::Shader::Loader(Renderer::geometry_shader_args));
        }

        Mat<4> compute_view_matrix() const;
        Mat<4> compute_proj_matrix() const;
        Mat<4> compute_view_proj() const;
        void configure(const engine::Window& window, engine::Scene& scene);

        std::vector<Mat<4>> collect_light_view_proj() const;
        void set_fog_uniforms(engine::Shader& shader) const;
        void set_shadow_uniforms(engine::Shader& shader) const;

        Vec<2> world_to_ndc(const Vec<3>& pos) const;

        void render_to_shadow_maps();
        void render_to_output();

        void render(
            engine::Mesh& mesh, 
            const engine::Texture& texture,
            const Mat<4>& local_transform = Mat<4>(),
            std::span<const Mat<4>> model_transforms
                = std::array<Mat<4>, 1> { Mat<4>() },
            engine::FaceCulling face_culling = engine::FaceCulling::Enabled,
            engine::Rendering rendering = engine::Rendering::Surfaces,
            engine::DepthTesting depth_testing = engine::DepthTesting::Enabled,
            std::optional<size_t> light_i = std::nullopt
        ) const;
        void render(
            engine::Model& model,
            std::span<const Mat<4>> model_transforms,
            engine::FaceCulling face_culling = engine::FaceCulling::Enabled,
            engine::Rendering rendering = engine::Rendering::Surfaces,
            engine::DepthTesting depth_testing = engine::DepthTesting::Enabled,
            const engine::Texture* override_texture = nullptr,
            std::optional<size_t> light_i = std::nullopt
        ) const;
        void render(
            engine::Model& model,
            std::span<const Mat<4>> model_transforms,
            const engine::Animation& animation,
            f64 timestamp,
            engine::FaceCulling face_culling = engine::FaceCulling::Enabled,
            engine::Rendering rendering = engine::Rendering::Surfaces,
            engine::DepthTesting depth_testing = engine::DepthTesting::Enabled,
            const engine::Texture* override_texture = nullptr,
            std::optional<size_t> light_i = std::nullopt
        ) const;

        const engine::Texture& output() const { return this->target; }

    };

}