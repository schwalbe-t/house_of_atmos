
#include "resources.hpp"
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>

namespace houseofatmos::engine::resources {


    std::string read_string(const char* file) {
        auto stream = std::ifstream(file);
        if(stream.fail()) {
            std::cout << "The file '" << file << "' could not be read!" 
                << std::endl;
            std::abort();
        }
        auto iter = std::istreambuf_iterator<char>(stream);
        return std::string(iter, std::istreambuf_iterator<char>());
    }


    engine::rendering::Mesh<ModelVertex> read_model(const char* file) {
        std::string content = read_string(file);
        auto lines = std::istringstream(content);
        auto positions = std::vector<Vec<3>>();
        auto uv_mappings = std::vector<Vec<2>>();
        auto normals = std::vector<Vec<3>>();
        auto mesh = engine::rendering::Mesh<ModelVertex>();
        size_t line_n = 0;
        for(std::string line; std::getline(lines, line); line_n += 1) {
            auto parts = std::istringstream(line);
            std::string type; 
            std::getline(parts, type, ' ');
            std::string a, b, c;
            if(type == "v" || type == "vn" || type == "f" || type == "vt") {
                std::getline(parts, a, ' ');
                std::getline(parts, b, ' ');
                if(type != "vt") {
                    std::getline(parts, c, ' ');
                }
            } else { continue; }
            if(type == "v") {
                positions.push_back(Vec<3>(stod(a), stod(b), stod(c)));
            } else if(type == "vn") {
                normals.push_back(Vec<3>(stod(a), stod(b), stod(c)));
            } else if(type == "vt") {
                uv_mappings.push_back(Vec<2>(stod(a), stod(b)));
            } else if(type == "f") {
                auto a_indices = std::istringstream(a);
                std::string a_pos; std::getline(a_indices, a_pos, '/');
                std::string a_uv; std::getline(a_indices, a_uv, '/');
                std::string a_norm; std::getline(a_indices, a_norm, '/');
                uint32_t a_idx = mesh.add_vertex({
                    positions[stoul(a_pos) - 1], 
                    uv_mappings[stoul(a_uv) - 1],
                    normals[stoul(a_norm) - 1]
                });
                auto b_indices = std::istringstream(b);
                std::string b_pos; std::getline(b_indices, b_pos, '/');
                std::string b_uv; std::getline(b_indices, b_uv, '/');
                std::string b_norm; std::getline(b_indices, b_norm, '/');
                uint32_t b_idx = mesh.add_vertex({
                    positions[stoul(b_pos) - 1], 
                    uv_mappings[stoul(b_uv) - 1],
                    normals[stoul(b_norm) - 1]
                });
                auto c_indices = std::istringstream(c);
                std::string c_pos; std::getline(c_indices, c_pos, '/');
                std::string c_uv; std::getline(c_indices, c_uv, '/');
                std::string c_norm; std::getline(c_indices, c_norm, '/');
                uint32_t c_idx = mesh.add_vertex({
                    positions[stoul(c_pos) - 1], 
                    uv_mappings[stoul(c_uv) - 1],
                    normals[stoul(c_norm) - 1]
                });
                mesh.add_element(a_idx, b_idx, c_idx);
            }
        }
        return mesh;
    }


    engine::rendering::Surface read_texture(const char* file) {
        Image img = LoadImage(file);
        if(!IsImageReady(img)) {
            std::cout << "The file '" << file << "' could not be read!"
                << std::endl;
            std::abort();
        }
        Color* data = LoadImageColors(img);
        auto surface = engine::rendering::Surface(data, nullptr, img.width, img.height);
        UnloadImageColors(data);
        UnloadImage(img);
        return surface;
    }


}