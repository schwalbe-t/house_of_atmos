
#include "engine/engine.hpp"

using namespace houseofatmos;
using namespace houseofatmos::engine::math;

struct CubeVertex {
    Vec<3> pos;
    Vec<3> color;
};

struct CubeShader: engine::rendering::Shader<CubeVertex, CubeShader> {
    Mat<4> projection;
    Mat<4> view;
    Mat<4> model;

    CubeShader() {}

    Vec<4> vertex(CubeVertex vertex) override {
        this->color = vertex.color;
        return this->projection * this->view * this->model 
            * vertex.pos.with(1.0);
    }

    Vec<3> color;

    Vec<4> fragment() override {
        this->interpolate(&this->color);
        return this->color.with(1.0);
    }
};

int main() {
    engine::init("House of Atmos", 800, 800, 60);
    auto main_buffer = engine::rendering::Surface(800, 800);
    auto sub_buffer = engine::rendering::Surface(800, 800);
    auto square = engine::rendering::Mesh<CubeVertex>();
    square.add_vertex({ Vec<3>(-1.0, -1.0,  1.0), Vec<3>(1.0, 0.0, 0.0) });
    square.add_vertex({ Vec<3>( 1.0, -1.0,  1.0), Vec<3>(0.0, 1.0, 0.0) });
    square.add_vertex({ Vec<3>( 1.0,  1.0,  1.0), Vec<3>(0.0, 0.0, 1.0) });
    square.add_vertex({ Vec<3>(-1.0,  1.0,  1.0), Vec<3>(1.0, 1.0, 0.0) });
    square.add_vertex({ Vec<3>(-1.0, -1.0, -1.0), Vec<3>(1.0, 0.0, 1.0) });
    square.add_vertex({ Vec<3>( 1.0, -1.0, -1.0), Vec<3>(0.0, 1.0, 1.0) });
    square.add_vertex({ Vec<3>( 1.0,  1.0, -1.0), Vec<3>(1.0, 1.0, 1.0) });
    square.add_vertex({ Vec<3>(-1.0,  1.0, -1.0), Vec<3>(0.0, 0.0, 0.0) });
    square.add_element(0, 1, 2);
    square.add_element(2, 3, 0);
    square.add_element(1, 5, 6);
    square.add_element(6, 2, 1);
    square.add_element(7, 6, 5);
    square.add_element(5, 4, 7);
    square.add_element(4, 0, 3);
    square.add_element(3, 7, 4);
    square.add_element(4, 5, 1);
    square.add_element(1, 0, 4);
    square.add_element(3, 2, 6);
    square.add_element(6, 7, 3);
    auto shader = CubeShader();
    shader.projection = Mat<4>::perspective(PI / 2.0, sub_buffer.width / sub_buffer.height, 0.1, 1000.0);
    //shader.projection = Mat<4>::orthographic(-3, 3, 3, -3, 0.1, 1000.0);
    shader.view = Mat<4>::look_at(Vec<3>(3, 0, 3), Vec<3>(0, 0, 0), Vec<3>(0, 1, 0));
    double rot_angle = 0.0;
    while(engine::is_running()) {
        rot_angle += GetFrameTime();
        main_buffer.clear();
        sub_buffer.clear();
        shader.model = Mat<4>::rotate_y(rot_angle)
            * Mat<4>::rotate_z(rot_angle);
        sub_buffer.draw_mesh(square, shader);
        main_buffer.blit_buffer(sub_buffer, 0, 0, 800, 800);
        engine::display_buffer(&main_buffer);
    }
    engine::stop();
}























