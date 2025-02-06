
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
        this->geometry_shader
            ->set_uniform("u_fog_start_dist", this->fog_start_dist);
        this->geometry_shader
            ->set_uniform("u_fog_gradiant_range", this->fog_gradiant_range);
        this->geometry_shader->set_uniform("u_fog_origin", this->fog_origin);
        this->geometry_shader
            ->set_uniform("u_fog_dist_scale", this->fog_dist_scale);
        this->geometry_shader->set_uniform("u_fog_color", this->fog_color);
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
                4096, 4096, this->lights.size()
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
        if(this->lights.size() > Renderer::max_light_c) {
            engine::debug("Scene has more lights than currently supported!");
        }
        this->geometry_shader->set_uniform(
            "u_light_count", (i64) this->lights.size()
        );
        std::vector<Mat<4>> light_view_proj;
        light_view_proj.reserve(this->lights.size());
        for(const DirectionalLight& light: this->lights) {
            light_view_proj.push_back(light.compute_view_proj());
        }
        this->geometry_shader->set_uniform("u_light_view_proj", light_view_proj);
        this->geometry_shader->set_uniform("u_shadow_maps", this->shadow_maps);
    }

    void Renderer::render(
        engine::Mesh& mesh, 
        const engine::Texture& texture,
        const Mat<4>& local_transform,
        std::span<const Mat<4>> model_transforms,
        bool wireframe, bool depth_test,
        std::optional<size_t> light_i
    ) const {
        bool render_all_light_maps = this->rendering_shadow_maps
            && !light_i.has_value() && !wireframe && depth_test; 
        if(render_all_light_maps) {
            for(size_t light_i = 0; light_i < this->lights.size(); light_i += 1) {
                this->render(
                    mesh, texture, local_transform, model_transforms,
                    false, true, light_i
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
        shader.set_uniform("u_joint_transfs", std::array { Mat<4>() });
        shader.set_uniform("u_texture", texture);
        engine::RenderTarget dest = light_i.has_value()
            ? this->shadow_maps.as_target(*light_i) : this->target.as_target();
        for(size_t completed = 0; completed < model_transforms.size();) {
            size_t remaining = model_transforms.size() - completed;
            size_t count = std::min(remaining, Renderer::max_inst_c);
            shader.set_uniform(
                "u_model_transfs", 
                model_transforms.subspan(completed, count)
            );
            mesh.render(shader, dest, count, wireframe, depth_test);
            completed += count;
        }
    }

    void Renderer::render(
        engine::Model& model,
        std::span<const Mat<4>> model_transforms,
        bool wireframe,
        const engine::Texture* override_texture,
        bool depth_test,
        std::optional<size_t> light_i
    ) const {
        bool render_all_light_maps = this->rendering_shadow_maps
            && !light_i.has_value() && !wireframe && depth_test; 
        if(render_all_light_maps) {
            for(size_t light_i = 0; light_i < this->lights.size(); light_i += 1) {
                this->render(
                    model, model_transforms, 
                    false, override_texture, true, light_i
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
        engine::RenderTarget dest = light_i.has_value()
            ? this->shadow_maps.as_target(*light_i) : this->target.as_target();
        for(size_t completed = 0; completed < model_transforms.size();) {
            size_t remaining = model_transforms.size() - completed;
            size_t count = std::min(remaining, Renderer::max_inst_c);
            shader.set_uniform(
                "u_model_transfs", 
                model_transforms.subspan(completed, count)
            );
            model.render_all(
                shader, dest, 
                "u_local_transf",
                override_texture == nullptr
                    ? std::optional("u_texture") : std::nullopt, 
                "u_joint_transfs",
                count, wireframe, depth_test
            );
            completed += count;
        }
    }

    void Renderer::render(
        engine::Model& model,
        std::span<const Mat<4>> model_transforms,
        const engine::Animation& animation,
        f64 timestamp,
        bool wireframe,
        const engine::Texture* override_texture,
        bool depth_test,
        std::optional<size_t> light_i
    ) const {
        bool render_all_light_maps = this->rendering_shadow_maps
            && !light_i.has_value() && !wireframe && depth_test; 
        if(render_all_light_maps) {
            for(size_t light_i = 0; light_i < this->lights.size(); light_i += 1) {
                this->render(
                    model, model_transforms, animation, timestamp,
                    false, override_texture, true, light_i
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
        engine::RenderTarget dest = light_i.has_value()
            ? this->shadow_maps.as_target(*light_i) : this->target.as_target();
        for(size_t completed = 0; completed < model_transforms.size();) {
            size_t remaining = model_transforms.size() - completed;
            size_t count = std::min(remaining, Renderer::max_inst_c);
            shader.set_uniform(
                "u_model_transfs", 
                model_transforms.subspan(completed, count)
            );
            model.render_all_animated(
                shader, dest,
                animation, timestamp,
                "u_joint_transfs", 
                "u_local_transf",
                override_texture == nullptr
                    ? std::optional("u_texture") : std::nullopt, 
                count, wireframe, depth_test
            );
            completed += count;
        }

    }

}