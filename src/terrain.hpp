
#pragma once

#include <druck/rendering.hpp>


namespace houseofatmos {

    namespace rendering = druck::rendering;
    using namespace druck::math;


    namespace water {

        struct Vertex {
            Vec<3> pos;
            Vec<2> uv;
        };

        struct Shader: rendering::Shader<Vertex, Shader> {
            static const int water_r = 82;
            static const int water_g = 115;
            static const int water_b = 221;

            Mat<4> projection;
            Mat<4> view;

            Vec<4> vertex(Vertex vertex) override {
                this->uv = vertex.uv;
                return this->projection * this->view * vertex.pos.with(1.0);
            }

            Vec<2> uv;

            Vec<4> fragment() override {
                this->interpolate(&this->uv);
                return Vec<4>(water_r, water_g, water_b, 255) * (1.0 / 255.0);
            }
        };

    }


    struct Terrain {

        enum Material {
            GRASS, SAND, DIRT
        };

        struct Vertex {
            Vec<3> pos;
            Vec<2> uv;
            Vec<3> normal;
            Material material;
        };

        struct Shader: rendering::Shader<Vertex, Shader> {
            Mat<4> projection;
            Mat<4> view;
            Vec<3> light;
            const rendering::Surface* grass; 
            const rendering::Surface* dirt;
            const rendering::Surface* sand;

            Vec<4> vertex(Vertex vertex) override {
                this->material = vertex.material;
                this->uv = vertex.uv;
                double diffuse = vertex.normal.dot((light - vertex.pos).normalized());
                diffuse += 1.0;
                diffuse /= 2.0;
                this->lightness = diffuse * 0.8 + 0.2;
                return this->projection * this->view * vertex.pos.with(1.0);
            }

            Terrain::Material material;
            Vec<2> uv;
            double lightness;

            Vec<4> fragment() override {
                this->flat(&this->material);
                this->interpolate(&this->uv);
                this->interpolate(&this->lightness);
                const rendering::Surface* texture;
                switch(this->material) {
                    case Terrain::Material::GRASS: texture = this->grass; break;
                    case Terrain::Material::DIRT: texture = this->dirt; break;
                    case Terrain::Material::SAND: texture = this->sand; break;
                }
                return texture->sample(uv) * lightness;
            }
        };

        static const size_t tile_count = 64; // 256;
        static const uint8_t tile_size = 10;
    
        int16_t height[tile_count + 1][tile_count + 1];
        rendering::Mesh<Terrain::Vertex> meshes[tile_count][tile_count];

        Terrain(uint32_t seed);

        void draw(rendering::Surface& surface, Shader& shader);  

    };

}
