
#include "resources.hpp"
#include "logging.hpp"
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;
namespace logging = houseofatmos::engine::logging;

namespace houseofatmos::engine::resources {


    template<typename T>
    static std::ifstream open_stream(T file) {
        auto stream = std::ifstream(file);
        if(stream.fail()) {
            logging::error(
                "The file '" + std::string(file) + "' could not be read"
            );
        }
        logging::info("Reading file '" + std::string(file) + "'");
        return stream;
    }


    std::string read_string(const char* file) {
        auto stream = open_stream(file);
        auto iter = std::istreambuf_iterator<char>(stream);
        return std::string(iter, std::istreambuf_iterator<char>());
    }


    rendering::Mesh<ModelVertex> read_model(const char* file) {
        std::string content = read_string(file);
        auto lines = std::istringstream(content);
        auto positions = std::vector<Vec<3>>();
        auto uv_mappings = std::vector<Vec<2>>();
        auto normals = std::vector<Vec<3>>();
        auto mesh = rendering::Mesh<ModelVertex>();
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
        logging::info("Reading file '" + std::string(file) + "'");
        Image img = LoadImage(file);
        if(!IsImageReady(img)) {
            logging::error(
                "The file '" + std::string(file) + "' could not be read"
            );
        }
        Color* data = LoadImageColors(img);
        auto surface = engine::rendering::Surface(data, nullptr, img.width, img.height);
        UnloadImageColors(data);
        UnloadImage(img);
        return surface;
    }


    engine::resources::RiggedModel read_rigged_model(const char* file) {
        fs::path parent_dir = fs::path(file).parent_path();
        auto stream = open_stream(file);
        json j = json::parse(stream);
        RiggedModel model;
        
        // read data buffers
        std::vector<std::vector<char>> buffers;
        for(size_t buff_i = 0; buff_i < j["buffers"].size(); buff_i += 1) {
            // read the buffer from the path (relative to the gltf file path)
            json buff_j = j["buffers"][buff_i];
            fs::path buff_file = parent_dir / fs::path(buff_j["uri"]);
            auto buff_stream = open_stream(buff_file);
            auto buffer = std::vector(
                std::istreambuf_iterator<char>(buff_stream), 
                std::istreambuf_iterator<char>()
            );
            // check that the buffer size matches the one the gltf file expects
            size_t expected_size = buff_j["byteLength"];
            if(buffer.size() != expected_size) {
                logging::warning(
                    "Buffer read from file '" + std::string(buff_file)
                        + "' has a size ("  + std::to_string(buffer.size())
                        + ") that does not match the expected size (" 
                        + std::to_string(expected_size)
                        + ") - is the file correct?"
                );
            }
            buffers.push_back(std::move(buffer));
        }
        
        // read materials and their textures
        for(size_t mat_i = 0; mat_i < j["materials"].size(); mat_i += 1) {
            // get the image path
            json mat_j = j["materials"][mat_i];
            int64_t texture_i = -1;
            if(mat_j.contains("emissiveTexture")) {
                texture_i = mat_j["emissiveTexture"]["index"];
            }
            if(mat_j.contains("baseColorTexture")) {
                texture_i = mat_j["baseColorTexture"]["index"];
            }
            if(texture_i == -1) {
                logging::error(
                    "The material '" + std::string(mat_j["name"])
                        + "' in the file '" + std::string(file)
                        + "' does not define a proper texture"
                );
            }
            uint64_t source_i = j["textures"][texture_i]["source"];
            json img_j = j["images"][source_i];
            fs::path img_file = parent_dir / fs::path(img_j["uri"]);
            // read the image
            rendering::Surface img = read_texture(img_file.string().c_str());
            model.textures.push_back(std::move(img));
        }
        
        // read bones
        std::abort(); // TODO!

        // read meshes
        std::abort(); // TODO!
        
        // read animations
        std::abort(); // TODO!
    }


}