
#include "resources.hpp"
#include <string>
#include <fstream>
#include <iostream>

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
        // TODO!
        std::cout << "not yet implemented" << std::endl;
        std::abort();
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