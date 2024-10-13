
#include "engine/engine.hpp"

using namespace houseofatmos;
using namespace houseofatmos::engine::math;

struct ModelShader: engine::rendering::Shader<engine::resources::ModelVertex, ModelShader> {
    Mat<4> projection;
    Mat<4> view;
    Mat<4> model;
    const engine::rendering::Surface* tex;
    Vec<3> light;
    double ambient;

    ModelShader() {}

    Vec<4> vertex(engine::resources::ModelVertex vertex) override {
        Vec<3> pos = (this->model * vertex.pos.with(1.0)).swizzle<3>("xyz");
        Vec<3> normal = (this->model * vertex.normal.with(1.0)).swizzle<3>("xyz");
        double reflected = normal.dot((this->light - pos).normalized());
        this->brightness = std::min(std::max(reflected, 0.0), 1.0) * (1.0 - this->ambient) 
            + this->ambient;
        this->uv = vertex.uv;
        return this->projection * this->view * this->model 
            * vertex.pos.with(1.0);
    }

    double brightness;
    Vec<2> uv;

    Vec<4> fragment() override {
        this->interpolate(&this->brightness);
        this->interpolate(&this->uv);
        return this->tex->sample(uv) * this->brightness;
    }
};

int main() {
    engine::init("House of Atmos", 1200, 800, 60);
    auto main_buffer = engine::rendering::Surface(1200, 800);
    auto sub_buffer = engine::rendering::Surface(300, 200);
    auto ramen_mesh = engine::resources::read_model("res/ramen.obj");
    auto ramen_tex = engine::resources::read_texture("res/ramen.png");
    auto model_shader = ModelShader();
    model_shader.projection = Mat<4>::perspective(PI / 2.0, sub_buffer.width, sub_buffer.height, 0.1, 1000.0);
    model_shader.view = Mat<4>::look_at(Vec<3>(0, 3, 3), Vec<3>(0, 0, 0), Vec<3>(0, 0, -1));
    model_shader.tex = &ramen_tex;
    model_shader.light = Vec<3>(5, 1.5, 5);
    model_shader.ambient = 0.6;
    double rot_angle = 0.0;
    while(engine::is_running()) {
        std::cout << 1.0 / GetFrameTime() << std::endl;
        rot_angle += GetFrameTime();
        main_buffer.clear();
        sub_buffer.clear();
        model_shader.model = Mat<4>::translate(Vec<3>(0.0, -1.0, 0.0))
           * Mat<4>::rotate_y(rot_angle);
        sub_buffer.draw_mesh(ramen_mesh, model_shader);
        main_buffer.blit_buffer(sub_buffer, 0, 0, 1200, 800);
        engine::display_buffer(&main_buffer);
    }
    engine::stop();
}























