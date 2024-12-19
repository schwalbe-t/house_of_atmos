
#include <engine/model.hpp>
#include <engine/logging.hpp>
#include <nlohmann/json.hpp>
#define TINYGLTF_NO_INCLUDE_JSON
#include <stb/stb_image.h>
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <syoyo/tiny_gltf.h>
#include <cassert>

namespace houseofatmos::engine {

    Model::Model() {}



    static void gltf_collect_textures(
        const tinygltf::Model& model, std::vector<Texture>& textures,
        const std::string& path
    ) {
        for(size_t mat_i = 0; mat_i < model.materials.size(); mat_i += 1) {
            const tinygltf::Material& material = model.materials[mat_i];
            int texture_id = material.emissiveTexture.index;
            if(texture_id == -1) {
                error("Materials are only supported if they have"
                    " an emissive texture. The material with ID "
                    + std::to_string(mat_i) + " in '" + path + "'"
                    " does not meet this condition"
                );
            }
            const tinygltf::Texture& texture = model.textures[texture_id];
            const tinygltf::Image& image = model.images[texture.source];
            textures.push_back(
                Texture(image.width, image.height, image.image.data())
            );
        }
    }


    static void gltf_build_node_to_joint(
        const tinygltf::Skin& skin,
        std::unordered_map<size_t, u16>& node_to_joint
    ) {
        for(size_t joint_i = 0; joint_i < skin.joints.size(); joint_i += 1) {
            node_to_joint[skin.joints[joint_i]] = joint_i;
        }
    }

    static void gltf_collect_bones(
        const tinygltf::Model& model, Animation::Skeleton& bones,
        std::unordered_map<size_t, u16>& node_to_joint,
        std::vector<bool>& bone_is_child,
        const std::string& path
    ) {
        if(model.skins.size() == 0) { return; }
        if(model.skins.size() > 1) {
            error("Currently only one skin (skeleton) per model is supported."
                " '" + path + "' does not meet this condition."
            );
        }
        const tinygltf::Skin& skin = model.skins[0];
        gltf_build_node_to_joint(skin, node_to_joint);
        // root will be figured out during actual tree pass
        bones.bones.resize(skin.joints.size());
        bone_is_child.resize(skin.joints.size());
        for(size_t joint_i = 0; joint_i < skin.joints.size(); joint_i += 1) {
            size_t joint_node_idx = skin.joints[joint_i];
            const tinygltf::Node& joint_node = model.nodes[joint_node_idx];
            auto children = std::vector<u16>();
            size_t child_node_c = joint_node.children.size();
            for(size_t child_i = 0; child_i < child_node_c; child_i += 1) {
                size_t child_node_idx = joint_node.children[child_i];
                if(!node_to_joint.contains(child_node_idx)) { continue; }
                size_t child_joint_i = node_to_joint[child_node_idx];
                bone_is_child[child_joint_i] = true;
                children.push_back(child_joint_i);                
            }
            bones.bones[joint_i] = std::move(children);
        }
    }


    static std::string gltf_attrib_name(Model::Attrib attrib) {
        switch(attrib) {
            case Model::Position: return "POSITION";
            case Model::UvMapping: return "TEXCOORD_0";
            case Model::Normal: return "NORMAL";
            case Model::Weights: return "WEIGHTS_0";
            case Model::Joints: return "JOINTS_0";
        }
        error("Unhandled attribute type in 'gltf_attrib_name'");
        return "<unknown>";
    }

    static void gltf_check_matching_attrib_sizes(
        int given_type, Mesh::Attrib expected,
        const std::string& attrib_name, size_t pmt_i, const std::string& path
    ) {
        size_t attrib_elems;
        switch(given_type) {
            case TINYGLTF_TYPE_VEC2: attrib_elems = 2; break;
            case TINYGLTF_TYPE_VEC3: attrib_elems = 3; break;
            case TINYGLTF_TYPE_VEC4: attrib_elems = 4; break;
            default: error("You really should never see this message :/");
        }
        if(attrib_elems >= expected.count) { return; }
        error("Expected attribute '"
            + attrib_name + "' on the primitive with ID "
            + std::to_string(pmt_i)
            + " to provide at least "
            + std::to_string(expected.count)
            + " values per vertex, but the model at '"
            + path + "' only provides "
            + std::to_string(attrib_elems)
        );
    }

    struct GltfBufferView {
        const u8* buffer;
        size_t stride;
        size_t count;
    };

    static GltfBufferView gltf_parse_indices(
        tinygltf::Model& model,
        const tinygltf::Primitive& primitive,
        const std::string& path
    ) {
        const tinygltf::Accessor& acc = model.accessors[primitive.indices];
        bool valid_type = acc.type == TINYGLTF_TYPE_SCALAR
            && acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT;
        if(!valid_type) {
            warning("Models must use 'u16' (unsigned short) for element indices"
                ", which '" + path + "' does not do. This may lead to a loss of data"
            );
        }
        if(acc.componentType == TINYGLTF_COMPONENT_TYPE_BYTE) {
            error("Model '" + path + "' uses 'u8' for element indices"
                ", which will result in undefined behavior"
            );
        }
        const tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];
        tinygltf::Buffer& buffer = model.buffers[bv.buffer];
        u8* ptr = buffer.data.data() + acc.byteOffset + bv.byteOffset;
        size_t stride = bv.byteStride == 0
            ? sizeof(u16)
            : bv.byteStride;
        return { ptr, stride, acc.count };
    }

    static GltfBufferView gltf_parse_mesh_attrib(
        tinygltf::Model& model, size_t mesh_id, 
        size_t pmt_i, const tinygltf::Primitive& primitive, 
        std::pair<Model::Attrib, Mesh::Attrib> attrib,
        const std::string& path
    ) {
        std::string attrib_name = gltf_attrib_name(attrib.first);
        if(!primitive.attributes.contains(attrib_name)) {
            error("Given model attributes expected an attribute '"
                + attrib_name + "' on the primitive with ID "
                + std::to_string(pmt_i) + " in the mesh with ID "
                + std::to_string(mesh_id) + " in '" + path + "'"
                ", but it does not exist"
            );
        }
        size_t acc_id = primitive.attributes.at(attrib_name);
        const tinygltf::Accessor& acc = model.accessors[acc_id];
        if(acc.componentType != attrib.second.gl_type_constant()) {
            error("Attribute '" + attrib_name + "' of primitive with ID "
                + std::to_string(pmt_i) + " in the mesh with ID "
                + std::to_string(mesh_id) + " in '" + path + "'"
                + " was expected to use type '"
                + attrib.second.display_type() 
                + "', but "
                + std::to_string(acc.componentType)
                + " (GL number constant) was used"
            );
        }
        gltf_check_matching_attrib_sizes(
            acc.type, attrib.second, attrib_name, pmt_i, path
        );
        const tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];
        tinygltf::Buffer& buffer = model.buffers[bv.buffer];
        u8* ptr = buffer.data.data() + acc.byteOffset + bv.byteOffset;
        size_t stride = bv.byteStride == 0
            ? attrib.second.size_bytes()
            : bv.byteStride;
        return { ptr, stride, acc.count };
    }

    static Mesh gltf_assemble_mesh(
        const Mat<4>& rotation, const Mat<4>& transform,
        const std::vector<std::pair<Model::Attrib, Mesh::Attrib>>& attribs,
        const std::vector<GltfBufferView>& attribs_data,
        GltfBufferView indices,
        const std::string& path
    ) {
        assert(attribs.size() == attribs_data.size());
        size_t vertices = attribs_data.size() > 0
            ? attribs_data[0].count
            : 0;
        std::vector<Mesh::Attrib> mesh_attribs;
        for(const auto& attrib: attribs) {
            mesh_attribs.push_back(attrib.second);
        }
        auto mesh = Mesh(mesh_attribs);
        for(size_t vert_i = 0; vert_i < vertices; vert_i += 1) {
            mesh.start_vertex();
            for(size_t attr_i = 0; attr_i < attribs.size(); attr_i += 1) {
                Model::Attrib model_attrib = attribs[attr_i].first;
                Mesh::Attrib mesh_attrib = attribs[attr_i].second;
                GltfBufferView attrib_data = attribs_data[attr_i];
                const u8* data = attrib_data.buffer
                    + vert_i * attrib_data.stride;
                size_t attrib_length = mesh_attrib.type_size_bytes()
                    * mesh_attrib.count;
                // // From testing it seems as though the models exported
                // // by Blender don't actually use the node transforms on the
                // // meshes, but only for bones
                // if(model_attrib == Model::Position && mesh_attrib.type == Mesh::F32) {
                //     size_t transf_len_bytes = sizeof(f32) * std::min(mesh_attrib.count, (size_t) 3);
                //     Vec<4, f32> transf = { 0, 0, 0, 1 };
                //     std::memcpy((void*) transf.elements, (void*) data, transf_len_bytes);
                //     transf = transform * transf;
                //     mesh.unsafe_put_raw(std::span((u8*) transf.elements, transf_len_bytes));
                //     data += transf_len_bytes;
                //     attrib_length -= transf_len_bytes;
                // }
                // if(model_attrib == Model::Normal && mesh_attrib.type == Mesh::F32) {
                //     size_t transf_len_bytes = sizeof(f32) * std::min(mesh_attrib.count, (size_t) 3);
                //     Vec<4, f32> transf = { 0, 0, 0, 1 };
                //     std::memcpy((void*) transf.elements, (void*) data, transf_len_bytes);
                //     transf = rotation * transf;
                //     mesh.unsafe_put_raw(std::span((u8*) transf.elements, transf_len_bytes));
                //     data += transf_len_bytes;
                //     attrib_length -= transf_len_bytes;
                // }
                mesh.unsafe_put_raw(std::span(data, attrib_length));
                mesh.unsafe_next_attr();
            }
            u16 vert_id = mesh.complete_vertex();
        }
        for(size_t elem_i = 0; elem_i < indices.count / 3; elem_i += 1) {
            const u8* start = indices.buffer + elem_i * 3 * indices.stride;
            mesh.add_element(
                *((u16*) (start + 0 * indices.stride)),
                *((u16*) (start + 1 * indices.stride)),
                *((u16*) (start + 2 * indices.stride))
            );
        }
        mesh.submit();
        return mesh;
    }

    static void gltf_collect_meshes(
        tinygltf::Model& model, size_t mesh_id, const std::string& name,
        const Mat<4>& rotation, const Mat<4>& transform,
        std::unordered_map<std::string, std::pair<std::unique_ptr<Mesh>, size_t>>& meshes,
        const std::vector<std::pair<Model::Attrib, Mesh::Attrib>>& attribs, 
        const std::string& path
    ) {
        const tinygltf::Mesh& mesh = model.meshes[mesh_id];
        for(size_t pmt_i = 0; pmt_i < mesh.primitives.size(); pmt_i += 1) {
            const tinygltf::Primitive& primitive = mesh.primitives[pmt_i];
            if(primitive.material == -1) {
                error("Mesh primitives are only supported if they have"
                    " a valid material. The primitive with ID "
                    + std::to_string(pmt_i) + " in the mesh with ID "
                    + std::to_string(mesh_id) + " in '" + path + "'"
                    " does not meet this condition"
                );
            }
            if(primitive.mode != 4 /* GL_TRIANGLES */) {
                error("Mesh primitives are only supported if they use the"
                    " 'GL_TRIANGLE' rendering mode. The primitive with ID "
                    + std::to_string(pmt_i) + " in the mesh with ID "
                    + std::to_string(mesh_id) + " in '" + path + "'"
                    " does not meet this condition"
                );
            }
            std::vector<GltfBufferView> attrib_data;
            for(size_t attrib_i = 0; attrib_i < attribs.size(); attrib_i += 1) {
                 attrib_data.push_back(gltf_parse_mesh_attrib(
                    model, mesh_id, pmt_i, primitive, attribs[attrib_i], 
                    path
                ));
            }
            GltfBufferView indices = gltf_parse_indices(model, primitive, path);
            const tinygltf::Material& material = model.materials[primitive.material];
            std::pair<std::unique_ptr<Mesh>, size_t> assembled = {
                std::make_unique<Mesh>(gltf_assemble_mesh(
                    rotation, transform, attribs, attrib_data, indices, path
                )),
                material.emissiveTexture.index
            };
            meshes[name] = std::move(assembled);
        }
    }


    static void gltf_find_root_bone(
        size_t node_idx, const Mat<4>& parent_transform,
        const std::unordered_map<size_t, u16>& node_to_joint, 
        std::vector<bool>& bone_is_child, Animation::Skeleton& skeleton
    ) {
        if(!node_to_joint.contains(node_idx)) { return; }
        size_t joint_i = node_to_joint.at(node_idx);
        if(bone_is_child[joint_i]) { return; }
        // this node is the root!
        skeleton.root_bone_idx = joint_i;
        skeleton.root_transform = parent_transform;
    }


    template<size_t N>
    static Vec<N> vec_from_vals_or(
        const std::vector<double>& values, Vec<N> dflt
    ) {
        if(values.size() == 0) { return dflt; }
        return Vec<N>(std::span(values));
    }

    static Mat<4> gltf_node_rotation(const tinygltf::Node& node) {
        return Mat<4>::quaternion_xyzw(vec_from_vals_or<4>(node.rotation, { 0, 0, 0, 1 }));
    }

    static Mat<4> gltf_node_transform(const tinygltf::Node& node, const std::string& path) {
        if(node.matrix.size() > 0) {
            error("Matrix transforms for GLTF nodes are currently unsupported"
                ", but were attempted to be used by '" + path + "'"
            );
        }
        return Mat<4>::translate(vec_from_vals_or<3>(node.translation, { 0, 0, 0 }))
            * gltf_node_rotation(node)
            * Mat<4>::scale(vec_from_vals_or<3>(node.scale, { 1, 1, 1 }));
    }

    static void gltf_collect_nodes(
        tinygltf::Model& model, const std::vector<int>& nodes,
        const Mat<4>& parent_rotation, const Mat<4>& parent_transform,
        std::unordered_map<std::string, std::pair<std::unique_ptr<Mesh>, size_t>>& collected_meshes,
        const std::unordered_map<size_t, u16>& node_to_joint, 
        std::vector<bool>& bone_is_child, Animation::Skeleton& skeleton,
        const std::vector<std::pair<Model::Attrib, Mesh::Attrib>>& attribs, 
        const std::string& path
    ) {
        for(size_t node_i = 0; node_i < nodes.size(); node_i += 1) {
            size_t node_idx = nodes[node_i];
            const tinygltf::Node& node = model.nodes[node_idx];
            Mat<4> rotation = gltf_node_rotation(node) * parent_rotation;
            Mat<4> transform = gltf_node_transform(node, path) * parent_transform;
            if(node.mesh != -1) {
                gltf_collect_meshes(
                    model, node.mesh, node.name,
                    rotation, transform, 
                    collected_meshes, attribs, path
                );
            }
            gltf_find_root_bone(
                node_idx, parent_transform, 
                node_to_joint, bone_is_child, skeleton
            );
            gltf_collect_nodes(
                model, node.children, 
                rotation, transform, 
                collected_meshes, node_to_joint, bone_is_child, skeleton,
                attribs, path
            );
        }
    }


    Model Model::from_resource(const Model::LoadArgs& args) {
        const std::string& path = args.path;
        const std::vector<std::pair<Attrib, Mesh::Attrib>>& attribs 
            = args.vertex_attributes;
        tinygltf::TinyGLTF loader;
        tinygltf::Model model;
        std::string warn;
        std::string err;
        bool success;
        if(path.ends_with(".gltf")) {
            success = loader.LoadASCIIFromFile(&model, &err, &warn, path);
        } else if(path.ends_with(".glb")) {
            success = loader.LoadBinaryFromFile(&model, &err, &warn, path);
        } else {
            error("'" + path + "' does not refer to a supported model type");
        }
        if(!warn.empty()) {
            warning("While loading '" + path + "': " + warn);
        }
        if(!err.empty()) {
            error("While loading '" + path + "': " + err);
        }
        if(!success) {
            error("Unable to parse '" + path + "'");
        }
        Model result;
        gltf_collect_textures(model, result.textures, path);
        std::unordered_map<size_t, u16> node_to_joint;
        std::vector<bool> bone_is_child;
        gltf_collect_bones(
            model, result.bones, node_to_joint, bone_is_child, path
        );
        tinygltf::Scene& scene = model.scenes[model.defaultScene];
        gltf_collect_nodes(
            model, scene.nodes, Mat<4>(), Mat<4>(), 
            result.meshes, node_to_joint, bone_is_child, result.bones,
            attribs, path
        );
        return result;
    }


    std::tuple<Mesh&, const Texture&> Model::primitive(
        const std::string& primitive_name
    ) {
        auto element = this->meshes.find(primitive_name);
        if(element == this->meshes.end()) {
           error("Model does not have primitive '" + primitive_name + "'");
        }
        Mesh& mesh = *element->second.first;
        const Texture& texture = this->textures.at(element->second.second);
        return { mesh, texture };
    }


    void Model::render_all(
        Shader& shader, const Texture& dest, std::string_view tex_uniform,
        bool depth_test
    ) {
        for(auto& [name, mesh]: this->meshes) {
            (void) name;
            shader.set_uniform(tex_uniform, this->textures[mesh.second]);
            mesh.first->render(shader, dest, depth_test);
        }
    }

}