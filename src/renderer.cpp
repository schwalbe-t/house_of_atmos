
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

    static void clear_output_texture(engine::Texture& output) {
        output.clear_color({ 0.1, 0.1, 0.1, 1.0 });
        output.clear_depth(INFINITY);
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
        return this->compute_proj_matrix() 
            * this->compute_view_matrix();
    }

    void Renderer::configure(
        const engine::Window& window, engine::Scene& scene
    ) {
        resize_output_texture(this->target, window, this->resolution);
        clear_output_texture(this->target);
        this->shader = &scene.get<engine::Shader>(Renderer::shader_args);
        this->shader->set_uniform("u_view_proj", this->compute_view_proj());
        this->shader->set_uniform("u_light_dir", this->light_direction.normalized());
        this->shader->set_uniform("u_ambient_light", this->ambient_light);
    }

    Vec<2> Renderer::world_to_ndc(const Vec<3>& pos) const {
        Mat<4> view_proj = this->compute_view_proj();
        Vec<4> ndc = view_proj * pos.with(1.0);
        ndc = ndc / ndc.w(); // perspective divide
        return ndc.swizzle<2>("xy");
    }


    void Renderer::render(
        engine::Mesh& mesh, 
        const engine::Texture& texture,
        const Mat<4>& local_transform,
        std::span<const Mat<4>> model_transforms,
        bool wireframe, bool depth_test
    ) const {
        this->shader->set_uniform("u_local_transf", local_transform);
        this->shader->set_uniform("u_joint_transfs", std::array { Mat<4>() });
        this->shader->set_uniform("u_texture", texture);
        for(size_t completed = 0; completed < model_transforms.size();) {
            size_t remaining = model_transforms.size() - completed;
            size_t count = std::min(remaining, Renderer::max_inst_c);
            this->shader->set_uniform(
                "u_model_transfs", 
                model_transforms.subspan(completed, count)
            );
            mesh.render(
                *this->shader, this->target, count, wireframe, depth_test
            );
            completed += count;
        }
    }

    void Renderer::render(
        engine::Model& model,
        std::span<const Mat<4>> model_transforms,
        bool wireframe,
        const engine::Texture* override_texture,
        bool depth_test
    ) const {
        if(override_texture != nullptr) {
            this->shader->set_uniform("u_texture", *override_texture);
        }
        for(size_t completed = 0; completed < model_transforms.size();) {
            size_t remaining = model_transforms.size() - completed;
            size_t count = std::min(remaining, Renderer::max_inst_c);
            this->shader->set_uniform(
                "u_model_transfs", 
                model_transforms.subspan(completed, count)
            );
            model.render_all(
                *this->shader, this->target, 
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
        bool depth_test
    ) const {
        if(override_texture != nullptr) {
            this->shader->set_uniform("u_texture", *override_texture);
        }
        for(size_t completed = 0; completed < model_transforms.size();) {
            size_t remaining = model_transforms.size() - completed;
            size_t count = std::min(remaining, Renderer::max_inst_c);
            this->shader->set_uniform(
                "u_model_transfs", 
                model_transforms.subspan(completed, count)
            );
            model.render_all_animated(
                *this->shader, this->target,
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