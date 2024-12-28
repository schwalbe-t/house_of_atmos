
#include "renderer.hpp"

namespace houseofatmos {

    static void resize_output_texture(
        engine::Texture& output, 
        const engine::Window& window,
        i64 resolution
    ) {
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

    Mat<4> Renderer::compute_view_proj() const {
        Mat<4> view = Mat<4>::look_at(
            this->camera.position, this->camera.look_at, this->camera.up
        );
        Mat<4> projection = Mat<4>::perspective(
            this->camera.fov, 
            this->target.width(), this->target.height(), 
            this->camera.near, this->camera.far 
        );
        return projection * view;
    }

    void Renderer::configure(
        const engine::Window& window, engine::Scene& scene
    ) {
        resize_output_texture(this->target, window, this->resolution);
        clear_output_texture(this->target);
        this->shader = &scene.get<engine::Shader>(Renderer::shader_args);
        this->shader->set_uniform(
            "u_view_projection", this->compute_view_proj()
        );
        this->shader->set_uniform("u_light_dir", this->light_direction.normalized());
        this->shader->set_uniform("u_ambient_light", this->ambient_light);
    }


    void Renderer::render(
        engine::Mesh& mesh, 
        const engine::Texture& texture,
        const Mat<4>& local_transform,
        std::span<const Mat<4>> model_transforms
    ) const {
        this->shader->set_uniform("u_local_transf", local_transform);
        this->shader->set_uniform("u_joint_transfs", std::array { Mat<4>() });
        this->shader->set_uniform("u_texture", texture);
        for(size_t offset = 0; offset < model_transforms.size();) {
            size_t remaining = model_transforms.size() - offset;
            size_t count = std::min(remaining, Renderer::max_inst_c);
            this->shader->set_uniform(
                "u_model_transfs", 
                model_transforms.subspan(offset, count)
            );
            mesh.render(*this->shader, this->target, count, true);
            offset += count;
        }
    }

    void Renderer::render(
        engine::Model& model,
        std::span<const Mat<4>> model_transforms
    ) const {
        for(size_t completed = 0; completed < model_transforms.size();) {
            size_t remaining = model_transforms.size() - completed;
            size_t count = std::min(remaining, Renderer::max_inst_c);
            this->shader->set_uniform(
                "u_model_transfs", 
                model_transforms.subspan(completed, count)
            );
            model.render_all(
                *this->shader, this->target, 
                "u_local_transf", "u_texture", "u_joint_transfs",
                count, true
            );
            completed += count;
        }
    }

    void Renderer::render(
        engine::Model& model,
        const Mat<4>& model_transform,
        const engine::Animation& animation,
        f64 timestamp
    ) const {
        this->shader->set_uniform(
            "u_model_transfs", 
            std::array { model_transform }
        );
        model.render_all_animated(
            *this->shader, this->target,
            animation, timestamp,
            "u_joint_transfs", "u_local_transf", "u_texture", 
            true
        );
    }

}