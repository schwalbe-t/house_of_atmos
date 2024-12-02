
#include "terrain.hpp"


namespace houseofatmos {

    static void generate_terrain_height(Terrain* terrain, uint32_t seed) {
        for(size_t x = 0; x <= Terrain::tile_count; x += 1) {
            for(size_t z = 0; z <= Terrain::tile_count; z += 1) {
                double height = 0.0;
                height += perlin_noise(seed, Vec<2>(x * 1.0, z * 1.0)) *  1.0;
                height += perlin_noise(seed, Vec<2>(x * 0.1, z * 0.1)) * 10.0;
                terrain->height[x][z] = floor(height);
            }
        }
    }

    static void generate_terrain_mesh(Terrain* terrain) {
        for(size_t x = 0; x < Terrain::tile_count; x += 1) {
            for(size_t z = 0; z < Terrain::tile_count; z += 1) {
                rendering::Mesh<Terrain::Vertex>& mesh = terrain->meshes[x][z];
                size_t offset = mesh.vertices.size();
                double top_left     = terrain->height[x    ][z    ];
                double top_right    = terrain->height[x + 1][z    ];
                double bottom_left  = terrain->height[x    ][z + 1];
                double bottom_right = terrain->height[x + 1][z + 1];
                double lowest = top_left;
                if(lowest > top_right   ) { lowest = top_right;    }
                if(lowest > bottom_left ) { lowest = bottom_left;  }
                if(lowest > bottom_right) { lowest = bottom_right; }
                mesh.add_vertex({ // [offset + 0] = top left
                    Vec<3>(x,     top_left,     z    ), Vec<2>(0.0, 1.0)
                });
                mesh.add_vertex({ // [offset + 1] = top right
                    Vec<3>(x + 1, top_right,    z    ), Vec<2>(1.0, 1.0)
                });
                mesh.add_vertex({ // [offset + 2] = bottom left
                    Vec<3>(x,     bottom_left,  z + 1), Vec<2>(0.0, 0.0)
                });
                mesh.add_vertex({ // [offset + 3] = bottom right
                    Vec<3>(x + 1, bottom_right, z + 1), Vec<2>(1.0, 0.0)
                });
                if(top_left == lowest || bottom_right == lowest) {
                    // [0]-[1]   ([0] or [3] is the lowest)
                    //  | / |
                    // [2]-[3]
                    mesh.add_element(offset + 0, offset + 1, offset + 2);
                    mesh.add_element(offset + 1, offset + 2, offset + 3);
                } else if(top_right == lowest || bottom_left == lowest) {
                    // [0]-[1]   ([1] or [2] is the lowest)
                    //  | \ |
                    // [2]-[3]
                    mesh.add_element(offset + 0, offset + 1, offset + 3);
                    mesh.add_element(offset + 0, offset + 2, offset + 3);
                }
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