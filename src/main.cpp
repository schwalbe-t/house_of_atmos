
#include <druck/window.hpp>
#include <druck/resources.hpp>
#include "terrain.hpp"

namespace rendering = druck::rendering;
namespace resources = druck::resources;
namespace animation = druck::animation;
using namespace druck::math;
using namespace houseofatmos;


struct RiggedModelShader: rendering::Shader<resources::RiggedModelVertex, RiggedModelShader> {
    Mat<4> projection;
    Mat<4> view;
    Mat<4> model;
    Mat<4> local;
    const std::vector<resources::RiggedModelBone>* bones;
    const rendering::Surface* texture;

    Vec<4> vertex(resources::RiggedModelVertex vertex) override {
        this->uv = vertex.uv;
        Vec<4> pos;
        for(size_t i = 0; i < 4; i += 1) {
            pos += this->bones->at(vertex.joints[i]).anim_transform 
                * vertex.pos.with(1.0) * vertex.weights[i];
        }
        return this->projection * this->view * this->model * this->local * pos;
    }

    Vec<2> uv;

    Vec<4> fragment() override {
        this->interpolate(&this->uv);
        return this->texture->sample(uv);
    }
};


int main() {
    druck::init("House of Atmos", 1200, 800, 60);
    auto main_buffer = rendering::Surface(1200, 800);
    auto sub_buffer = rendering::Surface(600, 400);
    auto terrain_grass = resources::read_texture("res/terrain/grass.png");
    auto terrain_sand = resources::read_texture("res/terrain/sand.png");
    auto terrain_dirt = resources::read_texture("res/terrain/dirt.png");
    auto terrain = Terrain(69);
    auto terrain_shader = Terrain::Shader();
    terrain_shader.projection = Mat<4>::perspective(pi / 2.0, sub_buffer.width, sub_buffer.height, 0.1, 1000.0);
    terrain_shader.texture = &terrain_grass;
    double theta = 0.0;
    while(druck::is_running()) {
        theta += GetFrameTime();
        const Vec<3> pos = Vec<3>(sin(theta) * 20, 20, cos(theta) * 20);
        terrain_shader.view = Mat<4>::look_at(pos, Vec<3>(0, 5, 0), Vec<3>(0, 1, 0));
        main_buffer.clear();
        sub_buffer.clear();
        terrain.draw(sub_buffer, terrain_shader);
        main_buffer.blit_buffer(sub_buffer, 0, 0, 1200, 800);
        druck::display_buffer(&main_buffer);
    }
    druck::stop();
}























