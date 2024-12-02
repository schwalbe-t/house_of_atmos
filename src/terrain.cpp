
#include "terrain.hpp"


namespace houseofatmos {

    static void generate_terrain_height(Terrain* terrain, uint32_t seed) {
        for(size_t x = 0; x <= Terrain::tile_count; x += 1) {
            for(size_t z = 0; z <= Terrain::tile_count; z += 1) {
                double height = 0.0;
                height += perlin_noise(seed, Vec<2>(x / 10.0, z / 10.0)) * 10;
                height += perlin_noise(seed, Vec<2>(x / 75.0, z / 75.0)) * 150
                    * perlin_noise(seed, Vec<2>(x / 300.0, z / 300.0));
                terrain->height[x][z] = floor(height * Terrain::tile_size)
                    / (double) Terrain::tile_size;
            }
        }
    }

    static double SAND_MAX_HEIGHT = 0.0;
    static double DIRT_MIN_HEIGHT_DIFF = 4.0;

    static Terrain::Material select_triangle_material(
        rendering::Mesh<Terrain::Vertex>& mesh,
        uint32_t idx_a, uint32_t idx_b, uint32_t idx_c
    ) {
        double h_a = mesh.vertices[idx_a].pos.y();
        double h_b = mesh.vertices[idx_b].pos.y();
        double h_c = mesh.vertices[idx_c].pos.y();
        bool is_water = h_a < SAND_MAX_HEIGHT || h_b < SAND_MAX_HEIGHT
            || h_c < SAND_MAX_HEIGHT;
        if(is_water) { return Terrain::Material::SAND; }
        double h_min = h_a;
        if(h_min > h_b) { h_min = h_b; }
        if(h_min > h_c) { h_min = h_c; }
        double h_max = h_a;
        if(h_max < h_b) { h_max = h_b; }
        if(h_max < h_c) { h_max = h_c; }
        bool is_cliff = h_max - h_min > DIRT_MIN_HEIGHT_DIFF;
        if(is_cliff) { return Terrain::Material::DIRT; }
        return Terrain::Material::GRASS;
    }

    static void set_triangle_materials(rendering::Mesh<Terrain::Vertex>& mesh) {
        for(size_t i = 0; i < mesh.elements.size(); i += 1) {
            std::tuple<uint32_t, uint32_t, uint32_t> element = mesh.elements[i];
            Terrain::Material mat = select_triangle_material(
                mesh, std::get<0>(element), std::get<1>(element), std::get<2>(element)
            );
            mesh.vertices[std::get<0>(element)].material = mat;
            mesh.vertices[std::get<1>(element)].material = mat;
            mesh.vertices[std::get<2>(element)].material = mat;
        }
    }

    static void generate_terrain_mesh(Terrain* terrain) {
        for(size_t x = 0; x < Terrain::tile_count; x += 1) {
            for(size_t z = 0; z < Terrain::tile_count; z += 1) {
                rendering::Mesh<Terrain::Vertex>& mesh = terrain->meshes[x][z];
                double h_top_left     = terrain->height[x    ][z    ];
                double h_top_right    = terrain->height[x + 1][z    ];
                double h_bottom_left  = terrain->height[x    ][z + 1];
                double h_bottom_right = terrain->height[x + 1][z + 1];
                Terrain::Vertex top_left = {
                    Vec<3>(
                        Terrain::tile_size * x,
                        h_top_left,
                        Terrain::tile_size * z
                    ),
                    Vec<2>(0.0, 1.0),
                    Vec<3>(), Terrain::Material::GRASS
                };
                Terrain::Vertex top_right = {
                    Vec<3>(
                        Terrain::tile_size * (x + 1), 
                        h_top_right,
                        Terrain::tile_size * z
                    ),
                    Vec<2>(1.0, 1.0),
                    Vec<3>(), Terrain::Material::GRASS
                };
                Terrain::Vertex bottom_left = {
                    Vec<3>(
                        Terrain::tile_size * x,
                        h_bottom_left,
                        Terrain::tile_size * (z + 1)
                    ), 
                    Vec<2>(0.0, 0.0), 
                    Vec<3>(), Terrain::Material::GRASS
                };
                Terrain::Vertex bottom_right = {
                    Vec<3>(
                        Terrain::tile_size * (x + 1), 
                        h_bottom_right, 
                        Terrain::tile_size * (z + 1)
                    ),
                    Vec<2>(1.0, 0.0), 
                    Vec<3>(), Terrain::Material::GRASS
                };
                Vec<3> left_norm;
                Vec<3> right_norm;
                double tl_to_br = fabs(h_top_left - h_bottom_right);
                double tr_to_bl = fabs(h_top_right - h_bottom_left);
                if(tl_to_br > tr_to_bl) {
                    // [ 0 ]-[1/3]
                    //   |  /  |
                    // [2/5]-[ 4 ]
                    mesh.add_vertex(top_left);
                    mesh.add_vertex(top_right);
                    mesh.add_vertex(bottom_left);
                    mesh.add_vertex(top_right);
                    mesh.add_vertex(bottom_right);
                    mesh.add_vertex(bottom_left);
                    left_norm = (bottom_left.pos - top_left.pos)
                        .cross(top_right.pos - top_left.pos).normalized();
                    right_norm = (bottom_left.pos - top_right.pos)
                        .cross(bottom_right.pos - top_right.pos).normalized();
                } else {
                    // [0/3]-[ 4 ]
                    //   |  \  |
                    // [ 2 ]-[1/5]
                    mesh.add_vertex(top_left);
                    mesh.add_vertex(bottom_right);
                    mesh.add_vertex(bottom_left);
                    mesh.add_vertex(top_left);
                    mesh.add_vertex(top_right);
                    mesh.add_vertex(bottom_right);
                    left_norm = (bottom_left.pos - top_left.pos)
                        .cross(bottom_right.pos - top_left.pos).normalized();
                    right_norm = (bottom_right.pos - top_left.pos)
                        .cross(top_right.pos - top_left.pos).normalized();
                }
                mesh.vertices[0].normal = left_norm;
                mesh.vertices[1].normal = left_norm;
                mesh.vertices[2].normal = left_norm;
                mesh.vertices[3].normal = right_norm;
                mesh.vertices[4].normal = right_norm;
                mesh.vertices[5].normal = right_norm;
                mesh.add_element(0, 1, 2);
                mesh.add_element(3, 4, 5);
                set_triangle_materials(mesh);
            }
        }
    }

    Terrain::Terrain(uint32_t seed) {
        generate_terrain_height(this, seed);
        generate_terrain_mesh(this);
    }

    void Terrain::draw(rendering::Surface& surface, Shader& shader) {
        for(size_t x = 0; x < Terrain::tile_count; x += 1) {
            for(size_t z = 0; z < Terrain::tile_count; z += 1) {
                surface.draw_mesh(this->meshes[x][z], shader);
            }
        }
    }

}