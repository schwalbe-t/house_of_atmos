
#pragma once

#include <druck/rendering.hpp>


namespace houseofatmos {

    namespace rendering = druck::rendering;
    using namespace druck::math;


    struct Terrain {

        struct Vertex {
            Vec<3> pos;
            Vec<2> uv;
        };

        struct Shader: rendering::Shader<Vertex, Shader> {
            Mat<4> projection;
            Mat<4> view;
            const rendering::Surface* texture;

            Vec<4> vertex(Vertex vertex) override {
                this->uv = vertex.uv;
                return this->projection * this->view * vertex.pos.with(1.0);
            }

            Vec<2> uv;

            Vec<4> fragment() override {
                this->interpolate(&this->uv);
                return this->texture->sample(uv);
            }
        };

        static const size_t tile_count = 32; // 256;
        static const uint8_t tile_size = 10;
    
        uint16_t height[tile_count + 1][tile_count + 1];
        rendering::Mesh<Terrain::Vertex> meshes[tile_count][tile_count];

        Terrain(uint32_t seed);

        void draw(rendering::Surface& surface, Shader& shader);

    };

}
