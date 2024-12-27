
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
        const Mat<4>& local_model_transform
    ) const {
        this->shader->set_uniform("u_local_transf", local_model_transform);
        this->shader->set_uniform("u_model_transf", Mat<4>());
        this->shader->set_uniform("u_joint_transf", std::vector { Mat<4>() });
        this->shader->set_uniform("u_texture", texture);
        mesh.render(*this->shader, this->target, true);
    }

    void Renderer::render(
        engine::Model& model,
        const Mat<4>& model_transform
    ) const {
        this->shader->set_uniform("u_model_transf", model_transform);
        model.render_all(
            *this->shader, this->target, 
            "u_local_transf", "u_texture", "u_joint_transf",
            true
        );
    }

}