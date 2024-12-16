
#include <engine/rendering.hpp>
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
        return { ptr, bv.byteStride, acc.count };
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
        return { ptr, bv.byteStride, acc.count };
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
                if(model_attrib == Model::Position && mesh_attrib.type == Mesh::F32) {
                    size_t transf_len = sizeof(f32) * std::min(mesh_attrib.count, (size_t) 3);
                    Vec<4> data_vec = {0, 0, 0, 1};
                    std::memcpy((void*) data_vec.elements, (void*) data, transf_len);
                    Vec<4> transformed = transform * data_vec;
                    mesh.unsafe_put_raw(std::span((u8*) transformed.elements, transf_len));
                    data += transf_len;
                    attrib_length -= transf_len;
                }
                if(model_attrib == Model::Normal && mesh_attrib.type == Mesh::F32) {
                    size_t transf_len = sizeof(f32) * std::min(mesh_attrib.count, (size_t) 3);
                    Vec<4> data_vec = {0, 0, 0, 1};
                    std::memcpy((void*) data_vec.elements, (void*) data, transf_len);
                    Vec<4> transformed = rotation * data_vec;
                    mesh.unsafe_put_raw(std::span((u8*) transformed.elements, transf_len));
                    data += transf_len;
                    attrib_length -= transf_len;
                }
                mesh.unsafe_put_raw(std::span(data, attrib_length));
                mesh.unsafe_next_attr();
            }
            u16 vert_id = mesh.complete_vertex();
        }
        for(size_t elem_i = 0; elem_i < indices.count / 3; elem_i += 1) {
            const u8* start = indices.buffer + elem_i * indices.stride;
            mesh.add_element(
                *((u16*) (start + 0 * indices.stride)),
                *((u16*) (start + 1 * indices.stride)),
                *((u16*) (start + 2 * indices.stride))
            );
        }
        return mesh;
    }

    static void gltf_collect_meshes(
        tinygltf::Model& model, size_t mesh_id, 
        const Mat<4>& rotation, const Mat<4>& transform,
        std::vector<std::pair<Mesh, size_t>>& meshes,
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
            std::pair<Mesh, size_t> assembled = {
                gltf_assemble_mesh(
                    rotation, transform, attribs, attrib_data, indices, path
                ),
                primitive.material  
            };
            meshes.push_back(std::move(assembled));
        }
    }


    template<size_t N>
    static Vec<N> vec_from_vals_or(
        const std::vector<double>& values, Vec<N> dflt
    ) {
        if(values.size() == 0) { return dflt; }
        return Vec<N>(values);
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
        std::vector<std::pair<Mesh, size_t>>& collected_meshes,
        const std::vector<std::pair<Model::Attrib, Mesh::Attrib>>& attribs, 
        const std::string& path
    ) {
        for(size_t node_i = 0; node_i < nodes.size(); node_i += 1) {
            const tinygltf::Node& node = model.nodes[nodes[node_i]];
            Mat<4> rotation = gltf_node_rotation(node) * parent_rotation;
            Mat<4> transform = gltf_node_transform(node, path) * parent_transform;
            if(node.mesh != -1) {
                gltf_collect_meshes(
                    model, node.mesh, rotation, transform, 
                    collected_meshes, attribs, path
                );
            }
            gltf_collect_nodes(
                model, node.children, 
                rotation, transform, 
                collected_meshes, 
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
        tinygltf::Scene& scene = model.scenes[model.defaultScene];
        gltf_collect_nodes(
            model, scene.nodes, Mat<4>(), Mat<4>(), 
            result.meshes, 
            attribs, path
        );
        return result;
    }



    void Model::render(
        Shader& shader, const Texture& dest, std::string_view tex_uniform
    ) {
        for(auto& [mesh, tex_id]: this->meshes) {
            shader.set_uniform(tex_uniform, this->textures[tex_id]);
            mesh.render(shader, dest);
        }
    }

}