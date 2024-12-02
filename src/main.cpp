
#include <druck/window.hpp>
#include <druck/resources.hpp>
#include <chrono>
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
    auto terrain = Terrain(std::chrono::system_clock::now().time_since_epoch().count());
    const Mat<4> projection = Mat<4>::perspective(pi / 2.0, sub_buffer.width, sub_buffer.height, 0.1, 1000.0);
    auto terrain_shader = Terrain::Shader();
    terrain_shader.projection = projection;
    terrain_shader.light = Vec<3>(0.0, 10000.0, 0.0);
    terrain_shader.grass = &terrain_grass;
    terrain_shader.dirt = &terrain_dirt;
    terrain_shader.sand = &terrain_sand;
    auto water_shader = water::Shader();
    water_shader.projection = projection;
    auto water_mesh = rendering::Mesh<water::Vertex>();
    double world_length = Terrain::tile_count * Terrain::tile_size;
    water_mesh.add_vertex({ Vec<3>(0.0,          -0.5, 0.0         ), Vec<2>(0.0, 1.0) });
    water_mesh.add_vertex({ Vec<3>(world_length, -0.5, 0.0         ), Vec<2>(1.0, 1.0) });
    water_mesh.add_vertex({ Vec<3>(0.0,          -0.5, world_length), Vec<2>(0.0, 0.0) });
    water_mesh.add_vertex({ Vec<3>(world_length, -0.5, world_length), Vec<2>(1.0, 0.0) });
    water_mesh.add_element(0, 1, 2);
    water_mesh.add_element(1, 2, 3);
    double time = pi / 4.0;
    while(druck::is_running()) {
        //time += GetFrameTime() / 4.0;
        const Vec<3> pos = Vec<3>(sin(time) * 320 + 320, 320, cos(time) * 320 + 320);
        const Mat<4> view = Mat<4>::look_at(pos, Vec<3>(320, 0, 320), Vec<3>(0, 1, 0));
        terrain_shader.view = view;
        water_shader.view = view;
        main_buffer.clear();
        sub_buffer.clear();
        terrain.draw(sub_buffer, terrain_shader);
        sub_buffer.draw_mesh(water_mesh, water_shader);
        main_buffer.blit_buffer(sub_buffer, 0, 0, 1200, 800);
        druck::display_buffer(&main_buffer);
    }
    druck::stop();
}























