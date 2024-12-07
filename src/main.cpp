
#include <druck/window.hpp>
#include <druck/resources.hpp>
#include <chrono>
#include "terrain.hpp"
#include "camera.hpp"

namespace window = druck::window;
namespace rendering = druck::rendering;
namespace resources = druck::resources;
namespace animation = druck::animation;
using namespace druck::math;
using namespace houseofatmos;


int main() {
    window::init("House of Atmos", 1200, 800, 60);
    auto main_buffer = rendering::Surface(1200, 800);
    auto sub_buffer = rendering::Surface(600, 400);
    auto camera = houseofatmos::Camera();

    auto terrain_grass = resources::read_texture("res/terrain/grass.png");
    auto terrain_sand = resources::read_texture("res/terrain/sand.png");
    auto terrain_dirt = resources::read_texture("res/terrain/dirt.png");
    auto terrain = Terrain(std::chrono::system_clock::now().time_since_epoch().count());
    auto terrain_shader = Terrain::Shader();
    terrain_shader.light = Vec<3>(0.0, 10000.0, 0.0);
    terrain_shader.grass = &terrain_grass;
    terrain_shader.dirt = &terrain_dirt;
    terrain_shader.sand = &terrain_sand;
    auto water_shader = water::Shader();
    auto water_mesh = rendering::Mesh<water::Vertex>();
    double world_length = Terrain::tile_count * Terrain::tile_size;
    water_mesh.add_vertex({ Vec<3>(0.0,          -0.5, 0.0         ), Vec<2>(0.0, 1.0) });
    water_mesh.add_vertex({ Vec<3>(world_length, -0.5, 0.0         ), Vec<2>(1.0, 1.0) });
    water_mesh.add_vertex({ Vec<3>(0.0,          -0.5, world_length), Vec<2>(0.0, 0.0) });
    water_mesh.add_vertex({ Vec<3>(world_length, -0.5, world_length), Vec<2>(1.0, 0.0) });
    water_mesh.add_element(0, 1, 2);
    water_mesh.add_element(1, 2, 3);

    double time = 0.0;
    while(!window::should_close()) {
        // configure buffers
        main_buffer.resize(window::size());
        main_buffer.clear();
        sub_buffer.resize(window::size() / 2);
        sub_buffer.clear();
        // update camera
        time += window::delta_time();
        double world_size = Terrain::tile_size * Terrain::tile_count;
        camera.pos = Vec<3>(
            sin(time) * world_size / 2 + world_size / 2, 
            world_size / 2, 
            cos(time) * world_size / 2 + world_size / 2
        );
        camera.look_at = Vec<3>(Terrain::tile_size * 32, 0, Terrain::tile_size * 32);
        camera.compute_matrices(sub_buffer.width, sub_buffer.height);
        // render terrain
        terrain_shader.view_proj = camera.view_proj;
        terrain.draw(sub_buffer, terrain_shader);
        water_shader.view_proj = camera.view_proj;
        sub_buffer.draw_mesh(water_mesh, water_shader);
        // display result
        main_buffer.blit_buffer(sub_buffer, 0, 0, main_buffer.width, main_buffer.height);
        window::display_buffer(main_buffer);
    }
    window::close();
}























