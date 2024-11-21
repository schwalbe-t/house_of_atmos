
#include "resources.hpp"
#include "logging.hpp"
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <unordered_set>

using json = nlohmann::json;
namespace fs = std::filesystem;
namespace logging = houseofatmos::engine::logging;
namespace animation = houseofatmos::engine::animation;

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

    static size_t find_gltf_joint_idx(json& joint_nodes, size_t node_idx) {
        for(size_t s_idx = 0; s_idx < joint_nodes.size(); s_idx += 1) {
            size_t s_node_idx = joint_nodes[s_idx];
            if(s_node_idx == node_idx) {
                return s_idx;
            }
        }
        std::abort();
    }

    static json& read_gltf_bones(
        json& j, std::vector<std::vector<char>>& buffers, const char* file,
        RiggedModel& model, json** root_node
    ) {
        size_t skin_count = j["skins"].size();
        if(skin_count == 0) { return j["skins"]; /* any empty array works */ }
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
                    size_t flat_i = column_i * 4 + row_i;
                    float value = view[joint_idx * 16 + flat_i];
                    bone.inverse_bind.element(row_i, column_i) = value;
                }
            }
        }
        // get the root bone
        json& scene = j["scenes"][(size_t) j["scene"]];
        *root_node = &j["nodes"][(size_t) scene["nodes"][0]];
        size_t root_bone_idx = joint_count;
        model.bones.push_back(RiggedModelBone());
        RiggedModelBone& root_bone = model.bones[root_bone_idx];
        model.root_bone_i = root_bone_idx;
        // read children for all joints
        for(size_t joint_idx = 0; joint_idx < joint_count; joint_idx += 1) {
            root_bone.children.push_back(joint_idx);
            RiggedModelBone& bone = model.bones[joint_idx];
            size_t node_idx = skin["joints"][joint_idx];
            json& node = j["nodes"][node_idx];
            size_t child_count = node["children"].size();
            for(size_t child_idx = 0; child_idx < child_count; child_idx += 1) {
                size_t child_node_idx = node["children"][child_idx];
                size_t child_joint_idx = find_gltf_joint_idx(
                    skin["joints"], child_node_idx
                );
                bone.children.push_back(child_joint_idx);
            }
        }
        // return the joints array for later usage
        return skin["joints"];
    }

    static char* get_gltf_view_ptr(
        json& j, std::vector<std::vector<char>>& buffers, size_t accessor_idx,
        size_t expected_component_type, const char* file
    ) {
        json& accessor = j["accessors"][accessor_idx];
        if(accessor["componentType"] != expected_component_type) {
            logging::error(
                "Expected component type " 
                    + std::to_string(expected_component_type)
                    + " for accessor with index "
                    + std::to_string(accessor_idx)
                    + " in file '" + std::string(file)
                    + "', but got component type "
                    + std::string(accessor["componentType"])
                    + ", which is either incorrect or currently not supported"
            );
        }
        size_t view_idx = accessor["bufferView"];
        json& view_j = j["bufferViews"][view_idx];
        size_t buffer_idx = view_j["buffer"];
        size_t byte_offset = view_j["byteOffset"];
        return &buffers[buffer_idx][byte_offset];
    }

    static void read_gltf_mesh(
        json& j, json& node, json& mesh_j, RiggedModel& model, const char* file,
        std::vector<std::vector<char>>& buffers
    ) {
        size_t mesh_idx = model.meshes.size();
        model.meshes.push_back(RiggedModelMesh());
        RiggedModelMesh& mesh = model.meshes[mesh_idx];
        mesh.texture = mesh_j["primitives"][0]["material"];
        // construct the local transformation matrix
        Mat<4>& lt = mesh.local_transform;
        if(node.contains("translation")) {
            json& translation = node["translation"];
            lt = lt * Mat<4>::translate(Vec<3>(
                translation[0], translation[1], translation[2]
            ));
        }
        if(node.contains("rotation")) {
            json& rotation = node["rotation"];
            lt = lt * Mat<4>::quaternion(
                rotation[0], rotation[1], rotation[2], rotation[3]
            );
        }
        if(node.contains("scale")) {
            json& scale = node["scale"];
            lt = lt * Mat<4>::scale(Vec<3>(
                scale[0], scale[1], scale[2]
            ));
        }
        // get pointers into the buffers for each vertex property
        json& attributes = mesh_j["primitives"][0]["attributes"];
        size_t indices_acc_idx = mesh_j["primitives"][0]["indices"];
        size_t position_acc_idx = attributes["POSITION"];
        size_t vertex_count = j["accessors"][position_acc_idx]["count"];
        float (*position_view)[3] = (float(*)[3]) get_gltf_view_ptr(
            j, buffers, position_acc_idx, 5126 /* GL_FLOAT */, file
        );
        float (*normal_view)[3] = (float(*)[3]) get_gltf_view_ptr(
            j, buffers, attributes["NORMAL"], 5126 /* GL_FLOAT */, file
        );
        float (*texcoord_view)[2] = (float(*)[2]) get_gltf_view_ptr(
            j, buffers, attributes["TEXCOORD_0"], 5126 /* GL_FLOAT */, file
        );
        uint8_t (*joints_view)[RIGGED_MESH_MAX_VERTEX_JOINTS] = NULL;
        if(attributes.contains("JOINTS_0")) {
            joints_view = (uint8_t(*)[RIGGED_MESH_MAX_VERTEX_JOINTS]) get_gltf_view_ptr(
                j, buffers, attributes["JOINTS_0"], 5121 /* GL_UNSIGNED_BYTE */, 
                file
            );
            std::string joints_type = j["accessors"][(size_t) attributes["JOINTS_0"]]["type"];
            if(joints_type != "VEC" + std::to_string(RIGGED_MESH_MAX_VERTEX_JOINTS)) {
                logging::error(
                    "File '" + std::string(file)
                        + "' uses an incorrect data type for vertex joints"
                        + " (expected 'VEC" 
                        + std::to_string(RIGGED_MESH_MAX_VERTEX_JOINTS)
                        + "', got '" + joints_type + "')"
                );
            }
        }
        float (*weights_view)[RIGGED_MESH_MAX_VERTEX_JOINTS] = NULL;
        if(attributes.contains("WEIGHTS_0")) {
            weights_view = (float(*)[RIGGED_MESH_MAX_VERTEX_JOINTS]) get_gltf_view_ptr(
                j, buffers, attributes["WEIGHTS_0"], 5126 /* GL_FLOAT */, file
            );
            std::string weights_type = j["accessors"][(size_t) attributes["WEIGHTS_0"]]["type"];
            if(weights_type != "VEC" + std::to_string(RIGGED_MESH_MAX_VERTEX_JOINTS)) {
                logging::error(
                    "File '" + std::string(file)
                        + "' uses an incorrect data type for vertex weights"
                        + " (expected 'VEC" 
                        + std::to_string(RIGGED_MESH_MAX_VERTEX_JOINTS)
                        + "', got '" + weights_type + "')"
                );
            }
        }
        // read the data into a single array
        for(size_t vert_i = 0; vert_i < vertex_count; vert_i += 1) {
            RiggedModelVertex vertex;
            float* view_pos = position_view[vert_i];
            vertex.pos = Vec<3>(view_pos[0], view_pos[1], view_pos[2]);
            float* view_norm = normal_view[vert_i];
            vertex.normal = Vec<3>(view_norm[0], view_norm[1], view_norm[2]);
            float* view_uv = texcoord_view[vert_i];
            // .gltf        - (0, 0) = top left
            // OpenGL, .obj - (0, 0) = bottom left
            // since we are using the OpenGL UV coordinate convention,
            // we need to flip the coordinates read from the file vertically
            vertex.uv = Vec<2>(view_uv[0], 1.0 - view_uv[1]);
            if(joints_view != NULL && weights_view != NULL) {
                vertex.joints[0] = joints_view[vert_i][0];
                vertex.joints[1] = joints_view[vert_i][1];
                vertex.joints[2] = joints_view[vert_i][2];
                vertex.joints[3] = joints_view[vert_i][3];
                vertex.weights[0] = weights_view[vert_i][0];
                vertex.weights[1] = weights_view[vert_i][1];
                vertex.weights[2] = weights_view[vert_i][2];
                vertex.weights[3] = weights_view[vert_i][3];
            }
            mesh.mesh.add_vertex(vertex);
        }
        // read the element data out of the buffers
        uint16_t* indices_view = (uint16_t*) get_gltf_view_ptr(
            j, buffers, indices_acc_idx, 5123 /* GL_UNSIGNED_SHORT */, file
        );
        size_t index_count = j["accessors"][indices_acc_idx]["count"];
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
            json& node = j["nodes"][(size_t) nodes[s_node_i]];
            if(node.contains("mesh")) {
                json& mesh = j["meshes"][(size_t) node["mesh"]];
                read_gltf_mesh(j, node, mesh, model, file, buffers);
            }
            collect_gltf_meshes(j, node["children"], model, file, buffers);
        }
    }

    template<int N>
    static Vec<N> get_gltf_node_property(
        json& node, const char* name, Vec<N> dflt
    ) {
        static_assert(N <= 4);
        if(!node.contains(name)) { return dflt; }
        auto r = Vec<N>();
        if(N >= 1) { r[0] = node[name][0]; }
        if(N >= 2) { r[1] = node[name][1]; }
        if(N >= 3) { r[2] = node[name][2]; }
        if(N >= 4) { r[3] = node[name][3]; }
        return r;
    }

    template<int N>
    static Vec<N> vec_from_float_arr(float* values) {
        auto r = Vec<N>();
        for(size_t i = 0; i < N; i += 1) {
            r[i] = values[i];
        }
        return r;
    }

    static void add_gltf_root_node_keyframe(
        json* root_node, animation::Animation& animation
    ) {
        auto root_keyframes = std::vector<animation::KeyFrame>();
        animation::KeyFrame root_keyframe;
        if(root_node->contains("translation")) {
            root_keyframe.translation = Vec<3>(
                root_node->at("translation")[0], 
                root_node->at("translation")[1], 
                root_node->at("translation")[2]
            );
        }
        root_keyframe.translation_itpl = animation::Interpolation::STEP;
        if(root_node->contains("rotation")) {
            root_keyframe.rotation = Vec<4>(
                root_node->at("rotation")[0], 
                root_node->at("rotation")[1], 
                root_node->at("rotation")[2], 
                root_node->at("rotation")[3]
            );
        }
        root_keyframe.rotation_itpl = animation::Interpolation::STEP;
        if(root_node->contains("scale")) {
            root_keyframe.scale = Vec<3>(
                root_node->at("scale")[0], 
                root_node->at("scale")[1], 
                root_node->at("scale")[2]
            );
        }
        root_keyframe.scale_itpl = animation::Interpolation::STEP;
        root_keyframes.push_back(root_keyframe);
        animation.keyframes.push_back(std::move(root_keyframes));
    }

    static void collect_gltf_keyframes(
        json& j, json& anim_j, json& joint_nodes, 
        std::vector<std::vector<char>>& buffers, const char* file,
        animation::Animation& animation
    ) {
        size_t joint_count = animation.keyframes.size();
        for(size_t joint_i = 0; joint_i < joint_count; joint_i += 1) {
            std::vector<animation::KeyFrame>& keyframes
                = animation.keyframes[joint_i];
            size_t joint_node_i = joint_nodes[joint_i];
            std::unordered_set<float> seen_timestamps;
            for(size_t ch_i = 0; ch_i < anim_j["channels"].size(); ch_i += 1) {
                json& channel_j = anim_j["channels"][ch_i];
                if(channel_j["target"]["node"] != joint_node_i) { continue; }
                size_t sampler_i = channel_j["sampler"];
                json& sampler_j = anim_j["samplers"][sampler_i];
                size_t ts_accessor_i = sampler_j["input"];
                size_t timestamp_count = j["accessors"][ts_accessor_i]["count"];
                float* timestamp_view = (float*) get_gltf_view_ptr(
                    j, buffers, ts_accessor_i, 5126 /* GL_FLOAT */, file
                );
                for(size_t ts_i = 0; ts_i < timestamp_count; ts_i += 1) {
                    float timestamp = timestamp_view[ts_i];
                    if(seen_timestamps.count(timestamp)) { continue; }
                    size_t insert_idx = keyframes.size();
                    for(size_t kf_i = 0; kf_i < keyframes.size(); kf_i += 1) {
                        if(keyframes[kf_i].timestamp < timestamp) { continue; }
                        insert_idx = kf_i;
                        break;
                    }
                    auto keyframe = animation::KeyFrame();
                    keyframe.timestamp = timestamp;
                    keyframes.insert(keyframes.begin() + insert_idx, keyframe);
                    seen_timestamps.insert(timestamp);
                }
            }
        }
    }

    static animation::Interpolation parse_gltf_interpolation_type(
        std::string interpolation_s, const char* file
    ) {
        if(interpolation_s == "STEP") {
            return animation::Interpolation::STEP;
        } else if(interpolation_s == "LINEAR") {
            return animation::Interpolation::LINEAR;
        } else {
            logging::error(
                "The file '" + std::string(file) 
                    + "' makes use of the animation interpolation type '"
                    + std::string(interpolation_s)
                    + "', which is currently not supported"
            );
            return animation::Interpolation::MISSING;
        }
    }

    static void read_gltf_keyframe_values(
        json& j, json& anim_j, json& joint_nodes,
        std::vector<std::vector<char>>& buffers, const char* file,
        animation::Animation& animation
    ) {
        for(size_t ch_i = 0; ch_i < anim_j["channels"].size(); ch_i += 1) {
            json& channel_j = anim_j["channels"][ch_i];
            std::string property_s = channel_j["target"]["path"];
            size_t node_idx = channel_j["target"]["node"];
            size_t joint_idx = find_gltf_joint_idx(joint_nodes, node_idx);
            size_t sampler_i = channel_j["sampler"];
            json& sampler_j = anim_j["samplers"][sampler_i];
            // parse the interpolation type
            std::string interpolation_s = sampler_j["interpolation"];
            animation::Interpolation interpolation
                = parse_gltf_interpolation_type(interpolation_s, file);
            // get the timestamp and value views
            size_t ts_accessor_i = sampler_j["input"];
            size_t timestamp_count = j["accessors"][ts_accessor_i]["count"];
            float* timestamp_view = (float*) get_gltf_view_ptr(
                j, buffers, ts_accessor_i, 5126 /* GL_FLOAT */, file
            );
            size_t value_accessor_i = sampler_j["output"];
            float* value_view = (float*) get_gltf_view_ptr(
                j, buffers, value_accessor_i, 5126 /* GL_FLOAT */, file
            );
            std::vector<animation::KeyFrame>& keyframes
                = animation.keyframes[joint_idx];
            size_t value_offset = 0;
            for(size_t ts_i = 0; ts_i < timestamp_count; ts_i += 1) {
                float timestamp = timestamp_view[ts_i];
                // find the keyframe to populate
                size_t keyframe_i;
                for(size_t s_kf_i = 0; s_kf_i < keyframes.size(); s_kf_i += 1) {
                    if(keyframes[s_kf_i].timestamp != timestamp) { continue; }
                    keyframe_i = s_kf_i;
                    break;
                }
                animation::KeyFrame& keyframe = keyframes[keyframe_i];
                // populate it with the data for that timestamp
                if(property_s == "translation") {
                    keyframe.translation_itpl = interpolation;
                    keyframe.translation = vec_from_float_arr<3>(
                        value_view + value_offset
                    );
                    value_offset += 3;
                } else if(property_s == "rotation") {
                    keyframe.rotation_itpl = interpolation;
                    keyframe.rotation = vec_from_float_arr<4>(
                        value_view + value_offset
                    );
                    value_offset += 4;
                } else if(property_s == "scale") {
                    keyframe.scale_itpl = interpolation;
                    keyframe.scale = vec_from_float_arr<3>(
                        value_view + value_offset
                    );
                    value_offset += 3;
                }
            }
        }
    }

    static void complete_first_gltf_keyframes(
        json& j, json& joint_nodes, animation::Animation& animation
    ) {
        size_t joint_count = animation.keyframes.size();
        for(size_t joint_i = 0; joint_i < joint_count; joint_i += 1) {
            std::vector<animation::KeyFrame>& keyframes
                = animation.keyframes[joint_i];
            if(keyframes.size() == 0) { continue; }
            size_t node_idx = joint_nodes[joint_i];
            json& node_j = j["nodes"][node_idx];
            animation::KeyFrame& f_frame = keyframes[0];
            if(f_frame.translation_itpl == animation::Interpolation::MISSING) {
                f_frame.translation = get_gltf_node_property<3>(
                    node_j, "translation", Vec<3>(0.0, 0.0, 0.0)
                );
                f_frame.translation_itpl = animation::Interpolation::STEP;
            }
            if(f_frame.rotation_itpl == animation::Interpolation::MISSING) {
                f_frame.rotation = get_gltf_node_property<4>(
                    node_j, "rotation", Vec<4>(0.0, 0.0, 0.0, 1.0)
                );
                f_frame.rotation_itpl = animation::Interpolation::STEP;
            } 
            if(f_frame.scale_itpl == animation::Interpolation::MISSING) {
                f_frame.scale = get_gltf_node_property<3>(
                    node_j, "scale", Vec<3>(1.0, 1.0, 1.0)
                );
                f_frame.scale_itpl = animation::Interpolation::STEP;
            }
        }
    }

    static animation::Animation read_gltf_animation(
        json& j, json& anim_j, json* root_node, 
        std::vector<std::vector<char>>& buffers,
        json& joint_nodes, const char* file
    ) {
        auto animation = animation::Animation();
        animation.keyframes = std::vector(
            joint_nodes.size(), std::vector<animation::KeyFrame>()
        );
        collect_gltf_keyframes(
            j, anim_j, joint_nodes, buffers, file, animation
        );
        read_gltf_keyframe_values(
            j, anim_j, joint_nodes, buffers, file, animation
        );
        complete_first_gltf_keyframes(
            j, joint_nodes, animation
        );
        add_gltf_root_node_keyframe(root_node, animation);
        animation.complete_keyframe_values();
        animation.compute_length();
        return animation;
    }

    static void read_gltf_animations(
        json& j, json* root_node, RiggedModel& model, 
        std::vector<std::vector<char>>& buffers, json& joint_nodes,
        const char* file
    ) {
        size_t anim_count = j["animations"].size();
        for(size_t anim_i = 0; anim_i < anim_count; anim_i += 1) {
            json& anim_j = j["animations"][anim_i];
            std::string anim_name = anim_j["name"];
            animation::Animation anim = read_gltf_animation(
                j, anim_j, root_node, buffers, joint_nodes, file
            );
            model.animations[anim_name] = anim;
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
        json* root_node;
        json& joint_nodes = read_gltf_bones(j, buffers, file, model, &root_node);
        json& scene = j["scenes"][(size_t) j["scene"]];
        collect_gltf_meshes(j, scene["nodes"], model, file, buffers);
        read_gltf_animations(j, root_node, model, buffers, joint_nodes, file);
        return model;
    }

}