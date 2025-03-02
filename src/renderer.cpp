
#include "renderer.hpp"

namespace houseofatmos {

    static void resize_output_texture(
        engine::Texture& output, 
        const engine::Window& window,
        u64 resolution
    ) {
        if(window.width() == 0 || window.height() == 0) { return; }
        if(window.height() < resolution) {
            output.resize_fast(window.width(), window.height());
        } else {
           f64 ratio = (f64) resolution / window.height();
           i64 width = ceil(window.width() * ratio);
           output.resize_fast(width, resolution);
        }
    }

    static void clear_output_texture(
        engine::RenderTarget output, const Vec<4>& color
    ) {
        output.clear_color(color);
        output.clear_depth(1.0);
    }

    Mat<4> Renderer::compute_view_matrix() const {
        return Mat<4>::look_at(
            this->camera.position, this->camera.look_at, this->camera.up
        );
    }

    Mat<4> Renderer::compute_proj_matrix() const {
        return Mat<4>::perspective(
            this->camera.fov, 
            this->target.width(), this->target.height(), 
            this->camera.near_plane, this->camera.far_plane
        );
    }

    Mat<4> Renderer::compute_view_proj() const {
        return this->compute_proj_matrix() * this->compute_view_matrix();
    }

    void Renderer::configure(
        const engine::Window& window, engine::Scene& scene
    ) {
        resize_output_texture(this->target, window, this->resolution);
        clear_output_texture(this->target.as_target(), this->fog_color);
        this->shadow_shader = &scene
            .get<engine::Shader>(Renderer::shadow_shader_args);
        this->geometry_shader = &scene
            .get<engine::Shader>(Renderer::geometry_shader_args);
        this->set_fog_uniforms(*this->geometry_shader);
    }

    std::vector<Mat<4>> Renderer::collect_light_view_proj() const {
        std::vector<Mat<4>> light_view_proj;
        light_view_proj.reserve(this->lights.size());
        for(const DirectionalLight& light: this->lights) {
            light_view_proj.push_back(light.compute_view_proj());
        }
        return light_view_proj;
    }

    void Renderer::set_fog_uniforms(engine::Shader& shader) const {
        shader.set_uniform("u_fog_start_dist", this->fog_start_dist);
        shader.set_uniform("u_fog_gradiant_range", this->fog_gradiant_range);
        shader.set_uniform("u_fog_origin", this->fog_origin);
        shader.set_uniform("u_fog_dist_scale", this->fog_dist_scale);
        shader.set_uniform("u_fog_color", this->fog_color);
    }

    void Renderer::set_shadow_uniforms(engine::Shader& shader) const {
        if(this->lights.size() > Renderer::max_light_c) {
            engine::warning("Scene has more lights than currently supported!");
        }
        shader.set_uniform("u_light_count", (i64) this->lights.size());
        shader.set_uniform("u_light_view_proj", this->collect_light_view_proj());
        shader.set_uniform("u_shadow_maps", this->shadow_maps);
    }

    Vec<2> Renderer::world_to_ndc(const Vec<3>& pos) const {
        Mat<4> view_proj = this->compute_view_proj();
        Vec<4> ndc = view_proj * pos.with(1.0);
        ndc = ndc / ndc.w(); // perspective divide
        return ndc.swizzle<2>("xy");
    }

    void Renderer::render_to_shadow_maps() {
        this->rendering_shadow_maps = true;
        if(this->shadow_maps.size() != this->lights.size()) {
            this->shadow_maps = engine::TextureArray(
                this->shadow_map_resolution, this->shadow_map_resolution, 
                this->lights.size()
            );
        }
        for(size_t light_i = 0; light_i < this->lights.size(); light_i += 1) {
            clear_output_texture(
                this->shadow_maps.as_target(light_i),
                { 1.0, 1.0, 1.0, 1.0 }
            );
        }
    }

    void Renderer::render_to_output() {
        this->rendering_shadow_maps = false;
        this->set_shadow_uniforms(*this->geometry_shader);
    }

    void Renderer::render(
        engine::Mesh& mesh, 
        const engine::Texture& texture,
        const Mat<4>& local_transform,
        std::span<const Mat<4>> model_transforms,
        std::span<const Mat<4>> joint_transforms,
        engine::FaceCulling face_culling,
        engine::Rendering rendering,
        engine::DepthTesting depth_testing,
        std::optional<size_t> light_i
    ) {
        bool render_all_light_maps = this->rendering_shadow_maps
            && !light_i.has_value() 
            && rendering == engine::Rendering::Surfaces 
            && depth_testing == engine::DepthTesting::Enabled; 
        if(render_all_light_maps) {
            for(size_t light_i = 0; light_i < this->lights.size(); light_i += 1) {
                this->render(
                    mesh, texture, local_transform, 
                    model_transforms, joint_transforms,
                    engine::FaceCulling::Disabled,
                    engine::Rendering::Surfaces,
                    engine::DepthTesting::Enabled, 
                    light_i
                );
            }
            return;
        }
        engine::Shader& shader = light_i.has_value()
            ? *this->shadow_shader : *this->geometry_shader;
        shader.set_uniform("u_view_proj", light_i.has_value()
            ? this->lights[*light_i].compute_view_proj() 
            : this->compute_view_proj()
        );
        shader.set_uniform("u_local_transf", local_transform);
        shader.set_uniform("u_joint_transfs", joint_transforms);
        shader.set_uniform("u_texture", texture);
        if(!light_i.has_value()) {
            shader.set_uniform("u_shadow_bias", this->shadow_bias);
        }
        engine::RenderTarget dest = light_i.has_value()
            ? this->shadow_maps.as_target(*light_i) : this->target.as_target();
        for(size_t completed = 0; completed < model_transforms.size();) {
            size_t remaining = model_transforms.size() - completed;
            size_t count = std::min(remaining, Renderer::max_inst_c);
            shader.set_uniform(
                "u_model_transfs", 
                model_transforms.subspan(completed, count)
            );
            mesh.render(
                shader, dest, count, face_culling, rendering, depth_testing
            );
            completed += count;
        }
    }

    void Renderer::render(
        engine::Model& model,
        std::span<const Mat<4>> model_transforms,
        const engine::Animation* animation,
        f64 timestamp,
        engine::FaceCulling face_culling,
        engine::Rendering rendering,
        engine::DepthTesting depth_testing,
        const engine::Texture* override_texture,
        std::optional<size_t> light_i
    ) {
        bool render_all_light_maps = this->rendering_shadow_maps
            && !light_i.has_value() 
            && rendering == engine::Rendering::Surfaces 
            && depth_testing == engine::DepthTesting::Enabled; 
        if(render_all_light_maps) {
            for(size_t light_i = 0; light_i < this->lights.size(); light_i += 1) {
                this->render(
                    model, model_transforms, animation, timestamp,
                    engine::FaceCulling::Disabled,
                    engine::Rendering::Surfaces,
                    engine::DepthTesting::Enabled, 
                    override_texture, light_i
                );
            }
            return;
        }
        engine::Shader& shader = light_i.has_value()
            ? *this->shadow_shader : *this->geometry_shader;
        shader.set_uniform("u_view_proj", light_i.has_value()
            ? this->lights[*light_i].compute_view_proj() 
            : this->compute_view_proj()
        );
        if(override_texture != nullptr) {
            shader.set_uniform("u_texture", *override_texture);
        }
        if(!light_i.has_value()) {
            shader.set_uniform("u_shadow_bias", this->shadow_bias);
        }
        engine::RenderTarget dest = light_i.has_value()
            ? this->shadow_maps.as_target(*light_i) : this->target.as_target();
        for(size_t completed = 0; completed < model_transforms.size();) {
            size_t remaining = model_transforms.size() - completed;
            size_t count = std::min(remaining, Renderer::max_inst_c);
            shader.set_uniform(
                "u_model_transfs", 
                model_transforms.subspan(completed, count)
            );
            if(animation == nullptr) {
                model.render_all(
                    shader, dest, "u_local_transf",
                    override_texture == nullptr
                        ? std::optional("u_texture") : std::nullopt,
                    "u_joint_transfs",
                    count, face_culling, rendering, depth_testing
                );
            } else {
                model.render_all_animated(
                    shader, dest,
                    *animation, timestamp,
                    "u_joint_transfs", 
                    "u_local_transf",
                    override_texture == nullptr
                        ? std::optional("u_texture") : std::nullopt, 
                    count, face_culling, rendering, depth_testing
                );
            }
            completed += count;
        }

    }

}