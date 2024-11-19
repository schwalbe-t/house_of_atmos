
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


    rendering::Mesh<ModelVertex> read_obj_model(const char* file) {
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


    static void read_gltf_buffers(
        json& j, fs::path& dir, std::vector<std::vector<char>>& buffers
    ) {
        for(size_t buff_i = 0; buff_i < j["buffers"].size(); buff_i += 1) {
            // read the buffer from the path (relative to the gltf file path)
            json& buff_j = j["buffers"][buff_i];
            fs::path buff_file = dir / fs::path(buff_j["uri"]);
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
    }

    static void read_gltf_materials(
        json& j, fs::path& dir, const char* file, RiggedModel& model
    ) {
        for(size_t mat_i = 0; mat_i < j["materials"].size(); mat_i += 1) {
            // get the image path
            json& mat_j = j["materials"][mat_i];
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
            json& img_j = j["images"][source_i];
            fs::path img_file = dir / fs::path(img_j["uri"]);
            // read the image
            rendering::Surface img = read_texture(img_file.string().c_str());
            model.textures.push_back(std::move(img));
        }
    }

    static void apply_inv_bind_matrix(
        Mat<4>* parent_inverse_bind, RiggedModel& model, RiggedModelBone& bone
    ) {
        if(parent_inverse_bind != NULL) {
            bone.inverse_bind = bone.inverse_bind * *parent_inverse_bind;
        }
        for(size_t child_i = 0; child_i < bone.children.size(); child_i += 1) {
            RiggedModelBone& child = model.bones[bone.children[child_i]];
            Mat<4> inverse_bind = bone.inverse_bind;
            apply_inv_bind_matrix(&inverse_bind, model, child);
        }
    }

    static void read_gltf_bones(
        json& j, std::vector<std::vector<char>>& buffers, const char* file,
        RiggedModel& model
    ) {
        size_t skin_count = j["skins"].size();
        if(skin_count == 0) { return; }
        if(skin_count > 1) {
            logging::error(
                "The file '" + std::string(file)
                    + "' defines multiple skins"
                      ", which this parser does not support."
            );
        }
        json& skin = j["skins"][0];
        // read the inverse bind matrices for all joints
        size_t joint_count = skin["joints"].size();
        size_t accessor_idx = skin["inverseBindMatrices"];
        json& accessor = j["accessors"][accessor_idx];
        size_t buffer_view_idx = accessor["bufferView"];
        json& buffer_view = j["bufferViews"][buffer_view_idx];
        size_t buffer_idx = buffer_view["buffer"];
        size_t byte_offset = buffer_view["byteOffset"];
        float* view = (float*) (&buffers[buffer_idx][byte_offset]);
        for(size_t joint_idx = 0; joint_idx < joint_count; joint_idx += 1) {
            model.bones.push_back(RiggedModelBone());
            RiggedModelBone& bone = model.bones[joint_idx];
            for(size_t row_i = 0; row_i < 4; row_i += 1) {
                for(size_t column_i = 0; column_i < 4; column_i += 1) {
                    size_t flat_i = row_i * 4 + column_i;
                    float value = view[joint_idx * 16 + flat_i];
                    bone.inverse_bind.element(row_i, column_i) = value;
                }
            }
        }
        // read children for all joints
        for(size_t joint_idx = 0; joint_idx < joint_count; joint_idx += 1) {
            RiggedModelBone& bone = model.bones[joint_idx];
            bone.has_parent = false;
            size_t node_idx = skin["joints"][joint_idx];
            json& node = j["nodes"][node_idx];
            size_t child_count = node["children"].size();
            for(size_t child_idx = 0; child_idx < child_count; child_idx += 1) {
                size_t child_node_idx = node["children"][child_idx];
                // search for the node index in the joints array
                size_t child_joint_idx;
                for(size_t s_idx = 0; s_idx < joint_count; s_idx += 1) {
                    size_t s_node_idx = skin["joints"][s_idx];
                    if(s_node_idx == child_node_idx) {
                        child_joint_idx = s_idx;
                        break;
                    }
                }
                // add the joint index as a child
                RiggedModelBone& child = model.bones[child_joint_idx];
                child.has_parent = true;
                bone.children.push_back(child_joint_idx);
            }
        }
        // apply the inverse bind matrix of all parents to their children
        for(size_t joint_idx = 0; joint_idx < joint_count; joint_idx += 1) {
            RiggedModelBone& bone = model.bones[joint_idx];
            if(bone.has_parent) { continue; }
            apply_inv_bind_matrix(NULL, model, bone);
        }
    }

    static void read_gltf_mesh(
        json& j, json& mesh_j, RiggedModel& model, const char* file,
        std::vector<std::vector<char>>& buffers
    ) {
        size_t mesh_idx = model.meshes.size();
        model.meshes.push_back(RiggedModelMesh());
        RiggedModelMesh& mesh = model.meshes[mesh_idx];
        mesh.texture = mesh_j["primitives"]["material"];
        json& attributes = mesh_j["primitives"]["attributes"];
        size_t indices_acc_idx = mesh_j["primitives"]["indices"];
        // get pointers into the buffers for each vertex property
        // position
        float (*position_view)[3];
        json& position_acc = j["accessors"][attributes["POSITION"]];
        json& position_view_j = j["bufferViews"][position_acc["bufferView"]];
        size_t position_buf_i = position_view_j["buffer"];
        size_t position_offset = position_view_j["byteOffset"];
        position_view = (float(*)[3]) &buffers[position_buf_i][position_offset];
        // normal
        float (*normal_view)[3];
        json& normal_acc = j["accessors"][attributes["NORMAL"]];
        json& normal_view_j = j["bufferViews"][normal_acc["bufferView"]];
        size_t normal_buf_i = normal_view_j["buffer"];
        size_t normal_offset = normal_view_j["byteOffset"];
        normal_view = (float(*)[3]) &buffers[normal_buf_i][normal_offset];
        // texcoord
        float (*texcoord_view)[2];
        json& texcoord_acc = j["accessors"][attributes["TEXCOORD_0"]];
        json& texcoord_view_j = j["bufferViews"][texcoord_acc["bufferView"]];
        size_t texcoord_buf_i = texcoord_view_j["buffer"];
        size_t texcoord_offset = texcoord_view_j["byteOffset"];
        texcoord_view = (float(*)[2]) &buffers[texcoord_buf_i][texcoord_offset];
        // joints
        uint8_t (*joints_view)[4] = NULL;
        if(attributes.contains("JOINTS_0")) {
            // TODO!
        }
        // weights
        float (*weigths_view)[4] = NULL;
        if(attributes.contains("WEIGHTS_0")) {
            // TODO!
        }
        // read the data into a single array
        size_t vertex_count = position_acc["count"];
        for(size_t vert_i = 0; vert_i < vertex_count; vert_i += 1) {
            RiggedModelVertex vertex;
            float* view_pos = position_view[vert_i];
            vertex.pos = Vec<3>(view_pos[0], view_pos[1], view_pos[2]);
            float* view_norm = normal_view[vert_i];
            vertex.normal = Vec<3>(view_norm[0], view_norm[1], view_norm[2]);
            float* view_uv = texcoord_view[vert_i];
            vertex.uv = Vec<2>(view_uv[0], view_uv[1]);
            if(joints_view != NULL && weigths_view != NULL) {
                // TODO!
            }
            mesh.mesh.add_vertex(vertex);
        }
        // read the element data out of the buffers
        json& indices_acc = j["accessors"][indices_acc_idx];
        if(indices_acc["componentType"] != 5123) {
            logging::error(
                "The file '" + std::string(file) 
                    + "' does not use component type 5123 (GL_UNSIGNED_SHORT)"
                    + " for its element indices!"
            );
        }
        json& indices_view_j = j["bufferViews"][indices_acc["bufferView"]];
        size_t ind_buf_i = indices_view_j["buffer"];
        size_t ind_offset = indices_view_j["byteOffset"];
        uint16_t* indices_view = (uint16_t*) &buffers[ind_buf_i][ind_offset];
        size_t index_count = indices_acc["count"];
        for(size_t idx_i = 0; idx_i < index_count; idx_i += 3) {
            mesh.mesh.add_element(
                indices_view[idx_i + 0],
                indices_view[idx_i + 1],
                indices_view[idx_i + 2]
            );
        }
    }

    static void collect_gltf_meshes(
        json& j, json& nodes, RiggedModel& model, const char* file,
        std::vector<std::vector<char>>& buffers
    ) {
        for(size_t s_node_i = 0; s_node_i < nodes.size(); s_node_i += 1) {
            json& node = j["nodes"][nodes[s_node_i]];
            // TODO! we need to apply rotation and translation
            //       to the meshes we collect!
            if(node.contains("mesh")) {
                json& mesh = j["meshes"][node["mesh"]];
                read_gltf_mesh(j, mesh, model, file, buffers);
            }
            collect_gltf_meshes(j, node["children"], model, file, buffers);
        }
    }

    engine::resources::RiggedModel read_gltf_model(const char* file) {
        fs::path dir = fs::path(file).parent_path();
        auto stream = open_stream(file);
        json j = json::parse(stream);
        RiggedModel model;
        std::vector<std::vector<char>> buffers;
        read_gltf_buffers(j, dir, buffers);
        read_gltf_materials(j, dir, file, model);
        read_gltf_bones(j, buffers, file, model);
        json& scene = j["scenes"][j["scene"]];
        collect_gltf_meshes(j, scene["nodes"], model, file, buffers);
        // read animations - TODO!
        return model;
    }


}