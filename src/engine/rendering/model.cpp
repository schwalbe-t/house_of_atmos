
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
                mesh.unsafe_put_raw(std::span(data, mesh_attrib.size_bytes()));
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

    static inline u64 primitive_key(u32 mesh_i, u32 pmt_i) {
        return ((u64) mesh_i << 32) | (u64) pmt_i;
    }

    static void gltf_collect_primitives(
        tinygltf::Model& model, std::vector<Mesh>& primitives,
        std::unordered_map<u64, size_t>& primitive_indices,
        const std::vector<std::pair<Model::Attrib, Mesh::Attrib>>& attribs,
        const std::string& path
    ) {
        for(size_t mesh_i = 0; mesh_i < model.meshes.size(); mesh_i += 1) {
            const tinygltf::Mesh& mesh = model.meshes[mesh_i];
            for(size_t pmt_i = 0; pmt_i < mesh.primitives.size(); pmt_i += 1) {
                const tinygltf::Primitive& primitive = mesh.primitives[pmt_i];
                if(primitive.material == -1) {
                    error("Mesh primitives are only supported if they have"
                        " a valid material. The primitive with ID "
                        + std::to_string(pmt_i) + " in the mesh with ID "
                        + std::to_string(mesh_i) + " in '" + path + "'"
                        " does not meet this condition"
                    );
                }
                if(primitive.mode != 4 /* GL_TRIANGLES */) {
                    error("Mesh primitives are only supported if they use the"
                        " 'GL_TRIANGLE' rendering mode. The primitive with ID "
                        + std::to_string(pmt_i) + " in the mesh with ID "
                        + std::to_string(mesh_i) + " in '" + path + "'"
                        " does not meet this condition"
                    );
                }
                std::vector<GltfBufferView> attrib_data;
                for(size_t attrib_i = 0; attrib_i < attribs.size(); attrib_i += 1) {
                    attrib_data.push_back(gltf_parse_mesh_attrib(
                        model, mesh_i, pmt_i, primitive, attribs[attrib_i], 
                        path
                    ));
                }
                GltfBufferView indices = gltf_parse_indices(model, primitive, path);
                const tinygltf::Material& material = model.materials[primitive.material];
                primitive_indices[primitive_key(mesh_i, pmt_i)]
                    = primitives.size();
                primitives.push_back(gltf_assemble_mesh(
                    attribs, attrib_data, indices, path
                ));
            }
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

    static void gltf_read_inverse_bind_matrices(
        const tinygltf::Model& model, const tinygltf::Skin& skin, size_t skin_i,
        Animation::Skeleton& skeleton,
        const std::string& path
    ) {
        if(skin.inverseBindMatrices == -1) {
            error("Skinned models must provide inverse bind matrices, "
                "which the skin with ID " + std::to_string(skin_i) 
                + " in '" + path + "' does not meet this condition"
            );
        }
        size_t acc_i = skin.inverseBindMatrices;
        const tinygltf::Accessor& acc = model.accessors[acc_i];
        bool valid_acc = acc.type == TINYGLTF_TYPE_MAT4
            && acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT;
        if(!valid_acc) {
            error("Inverse bind matrix accessors must be of type 'MAT4'"
                " and component type " 
                + std::to_string(TINYGLTF_COMPONENT_TYPE_FLOAT)
                + ", but the accessor with ID " + std::to_string(acc_i)
                + " for the skin with ID " + std::to_string(skin_i)
                + " in '" + path + "' does not meet this condition"
            );
        }
        if(acc.count != skeleton.bones.size()) {
            error("Expected the accessor to provide values for "
                + std::to_string(skeleton.bones.size()) + " bone(s)"
                + ", but the accessor with ID " + std::to_string(acc_i)
                + " for the skin with ID " + std::to_string(skin_i)
                + " in '" + path + "' provides " + std::to_string(acc.count)
            );
        }
        const tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];
        size_t stride = bv.byteStride == 0
            ? sizeof(f32) * 4 * 4
            : bv.byteStride;
        const tinygltf::Buffer& buff = model.buffers[bv.buffer];
        const u8* data = buff.data.data() + bv.byteOffset + acc.byteOffset;
        for(size_t bone_i = 0; bone_i < skeleton.bones.size(); bone_i += 1) {
            const f32* values = (f32*) (data + stride * bone_i);
            skeleton.bones[bone_i].inverse_bind 
                = Mat<4>::from_column_major<f32>(std::span(values, 4 * 4));
        }
    }

    static void gltf_collect_skeleton(
        const tinygltf::Model& model, const tinygltf::Skin& skin, size_t skin_i,
        Animation::Skeleton& skeleton,
        std::unordered_map<size_t, u16>& node_to_joint,
        const std::string& path
    ) {
        gltf_build_node_to_joint(skin, node_to_joint);
        skeleton.bones.resize(skin.joints.size());
        for(size_t joint_i = 0; joint_i < skin.joints.size(); joint_i += 1) {
            size_t joint_node_idx = skin.joints[joint_i];
            const tinygltf::Node& joint_node = model.nodes[joint_node_idx];
            auto children = std::vector<u16>();
            size_t child_node_c = joint_node.children.size();
            for(size_t child_i = 0; child_i < child_node_c; child_i += 1) {
                size_t child_node_idx = joint_node.children[child_i];
                if(!node_to_joint.contains(child_node_idx)) { continue; }
                size_t child_joint_i = node_to_joint[child_node_idx];
                children.push_back(child_joint_i);                
            }
            skeleton.bones[joint_i] = {
                Mat<4>(),
                std::vector(std::move(children))
            };
        }
        gltf_read_inverse_bind_matrices(model, skin, skin_i, skeleton, path);
        // root node and transform will be figured out during actual tree pass
        skeleton.root_bone_idx = skeleton.bones.size();
        skeleton.bones.push_back({ Mat<4>(), std::vector<u16>() });
    }

    static void gltf_collect_skeletons(
        const tinygltf::Model& model, 
        std::vector<Animation::Skeleton>& skeletons,
        std::vector<std::unordered_map<size_t, u16>>& node_to_joint,
        const std::string& path
    ) {
        skeletons.resize(model.skins.size());
        node_to_joint.resize(model.skins.size());
        for(size_t skin_i = 0; skin_i < model.skins.size(); skin_i += 1) {
            const tinygltf::Skin& skin = model.skins[skin_i];
            gltf_collect_skeleton(
                model, skin, skin_i, 
                skeletons.at(skin_i), node_to_joint.at(skin_i), 
                path
            );
        }
    }



    static size_t gltf_find_anim_target_skeleton(
        size_t node_i,
        const std::vector<std::unordered_map<size_t, u16>>& node_to_joint,
        const std::string& anim_name, const std::string& path
    ) {
        for(size_t skin_i = 0; skin_i < node_to_joint.size(); skin_i += 1) {
            const std::unordered_map<size_t, u16>& nodes = node_to_joint[skin_i];
            if(!nodes.contains(node_i)) { continue; }
            return skin_i;
        }
        error("The animation '" + anim_name + "' in '" + path + "' refers to a"
            " node that is not part of any skeleton"
        );
        return 0;
    }

    static Animation::Interpolation gltf_parse_interp_type(
        const std::string& type, 
        const std::string& anim_name, const std::string& path
    ) {
        if(type == "STEP") { return Animation::Step; }
        if(type == "LINEAR") { return Animation::Linear; }
        error("Currently, only the animation interpolation types 'STEP'"
            " and 'LINEAR' are supported. The animation '" + anim_name + "'"
            " in '" + path + "' does not meet this condition"
        );
        return Animation::Step;
    }

    template<size_t N>
    static std::vector<Animation::KeyFrame<N>> gltf_read_keyframes(
        const tinygltf::Model& model, const tinygltf::AnimationSampler& sampler,
        size_t sampler_i, f64& last_timestamp,
        const std::string& anim_name, const std::string& path
    ) {
        Animation::Interpolation interp_type
            = gltf_parse_interp_type(sampler.interpolation, anim_name, path);
        std::vector<Animation::KeyFrame<N>> frames;
        { // timestamps
            size_t acc_i = sampler.input;
            const tinygltf::Accessor& acc = model.accessors[acc_i];
            bool acc_valid = acc.type == TINYGLTF_TYPE_SCALAR
                && acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT;
            if(!acc_valid) {
                error("Animation channel input accessors must use type 'SCALAR'"
                    " and component type "
                    + std::to_string(TINYGLTF_COMPONENT_TYPE_FLOAT)
                    + "(GL integer constant), but the sampler with ID" 
                    + std::to_string(sampler_i) + "' in the animation '" + anim_name
                    + "' does not meet this condition"
                );
            }
            const tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];
            size_t stride = bv.byteStride == 0
                ? sizeof(f32) : bv.byteStride;
            const tinygltf::Buffer& buff = model.buffers[bv.buffer];
            const u8* data = buff.data.data() + acc.byteOffset + bv.byteOffset;
            frames.resize(acc.count);
            for(size_t kf_i = 0; kf_i < frames.size(); kf_i += 1) {
                const f32* timestamp = (const f32*) (data + kf_i * stride);
                last_timestamp = std::max(last_timestamp, (f64) *timestamp);
                frames[kf_i] = { *timestamp, Vec<N>(), interp_type };
            }
        }
        { // values
            size_t acc_i = sampler.output;
            const tinygltf::Accessor& acc = model.accessors[acc_i];
            bool acc_valid = acc.type == (TINYGLTF_TYPE_VEC2 - 2 + N) 
                && acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT
                && acc.count == frames.size();
            if(!acc_valid) {
                error("Animation channel output accessor was expected to use"
                    " type 'VEC" + std::to_string(N) + "', use"
                    " component type " + std::to_string(TINYGLTF_COMPONENT_TYPE_FLOAT)
                    + " (GL integer constant) and provide "
                    + std::to_string(frames.size()) + " values,"
                    " but the sampler with ID"
                    + std::to_string(sampler_i) + " in the animation '"
                    + anim_name + "' does not meet this condition"
                );
            }
            const tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];
            size_t stride = bv.byteStride == 0
                ? sizeof(f32) * N : bv.byteStride;
            const tinygltf::Buffer& buff = model.buffers[bv.buffer];
            const u8* data = buff.data.data() + acc.byteOffset + bv.byteOffset;
            for(size_t kf_i = 0; kf_i < frames.size(); kf_i += 1) {
                const f32* values = (const f32*) (data + kf_i * stride);
                frames[kf_i].value = Vec<N>(std::span(values, N));
            }
        }
        return frames;
    }

    static void gltf_collect_animations(
        tinygltf::Model& model,
        std::unordered_map<std::string, Animation>& animations,
        const std::vector<std::unordered_map<size_t, u16>>& node_to_joint,
        const std::string& path
    ) {
        for(size_t anim_i = 0; anim_i < model.animations.size(); anim_i += 1) {
            tinygltf::Animation& animation = model.animations[anim_i];
            std::vector<Animation::Channel> r_channels;
            f64 last_timestamp = 0;
            if(animation.channels.size() == 0) { continue; }
            size_t skin_i = gltf_find_anim_target_skeleton(
                animation.channels[0].target_node, node_to_joint, 
                animation.name, path
            );
            r_channels.resize(node_to_joint.at(skin_i).size());
            for(size_t ch_i = 0; ch_i < animation.channels.size(); ch_i += 1) {
                const tinygltf::AnimationChannel& channel = animation.channels[ch_i];
                size_t joint_i = node_to_joint.at(skin_i).at(channel.target_node);
                const tinygltf::AnimationSampler& sampler
                    = animation.samplers[channel.sampler];
                if(channel.target_path == "translation") {
                    r_channels[joint_i].translation = gltf_read_keyframes<3>(
                        model, sampler, channel.sampler, last_timestamp, 
                        animation.name, path
                    );
                } else if(channel.target_path == "rotation") {
                    r_channels[joint_i].rotation = gltf_read_keyframes<4>(
                        model, sampler, channel.sampler, last_timestamp, 
                        animation.name, path
                    );
                } else if(channel.target_path == "scale") {
                    r_channels[joint_i].scale = gltf_read_keyframes<3>(
                        model, sampler, channel.sampler, last_timestamp,
                        animation.name, path
                    );
                } else {
                    error("'" + channel.target_path + "' is not a supported"
                        " channel path, but '" + path + "' uses it"
                    );
                }
            }
            animations[animation.name]
                = Animation(std::move(r_channels), last_timestamp);
        }
    }



    static void gltf_collect_meshes(
        tinygltf::Model& model, size_t mesh_i, i64 skin_i,
        const std::string& name,
        std::unordered_map<u64, size_t>& primitive_indices,
        std::unordered_map<std::string, std::tuple<size_t, size_t, std::optional<size_t>>>& meshes
    ) {
        const tinygltf::Mesh& mesh = model.meshes[mesh_i];
        for(size_t pmt_i = 0; pmt_i < mesh.primitives.size(); pmt_i += 1) {
            const tinygltf::Primitive& primitive = mesh.primitives[pmt_i];
            const tinygltf::Material& material = model.materials[primitive.material];
            size_t texture_i = material.emissiveTexture.index;
            size_t mesh_abs_i = primitive_indices[primitive_key(mesh_i, pmt_i)];
            std::optional<size_t> skeleton_i = skin_i == -1
                ? std::optional(skin_i)
                : std::nullopt;
            meshes[name] = { mesh_abs_i, texture_i, mesh_abs_i };
        }
    }

    static void gltf_find_root_bone(
        tinygltf::Model& model, size_t node_idx, const Mat<4>& transform,
        std::vector<std::unordered_map<size_t, u16>>& nodes_to_joints, 
        std::vector<Animation::Skeleton>& skeletons
    ) {
        assert(nodes_to_joints.size() == skeletons.size());
        for(size_t skin_i = 0; skin_i < skeletons.size(); skin_i += 1) {
            auto& node_to_joint = nodes_to_joints.at(skin_i);
            auto& skeleton = skeletons.at(skin_i);
            auto& root_bone = skeleton.bones.at(skeleton.root_bone_idx);
            if(root_bone.child_indices.size() > 0) { continue; }
            if(node_to_joint.contains(node_idx)) { continue; }
            const tinygltf::Node& node = model.nodes[node_idx];
            for(auto child_node_idx: node.children) {
                if(!node_to_joint.contains(child_node_idx)) { continue; }
                u16 child_joint_i = node_to_joint.at(child_node_idx);
                root_bone.child_indices.push_back(child_joint_i);
            }
            if(root_bone.child_indices.size() == 0) { continue; }
            skeleton.root_transform = transform;
            node_to_joint[node_idx] = skeleton.root_bone_idx;
        }
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
        const Mat<4>& parent_transform,
        std::unordered_map<u64, size_t>& primitive_indices,
        std::unordered_map<std::string, std::tuple<size_t, size_t, std::optional<size_t>>>& collected_meshes,
        std::vector<std::unordered_map<size_t, u16>>& node_to_joint, 
        std::vector<Animation::Skeleton>& skeletons,
        const std::string& path
    ) {
        for(size_t node_i = 0; node_i < nodes.size(); node_i += 1) {
            size_t node_idx = nodes[node_i];
            const tinygltf::Node& node = model.nodes[node_idx];
            Mat<4> transform = gltf_node_transform(node, path) * parent_transform;
            if(node.mesh != -1) {
                gltf_collect_meshes(
                    model, node.mesh, node.skin, node.name, 
                    primitive_indices, collected_meshes
                );
            }
            gltf_find_root_bone(
                model, node_idx, transform, 
                node_to_joint, skeletons
            );
            gltf_collect_nodes(
                model, node.children, 
                transform, 
                primitive_indices, collected_meshes, 
                node_to_joint, skeletons,
                path
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
        std::unordered_map<size_t, size_t> primitive_indices;
        gltf_collect_primitives(model, result.primitives, primitive_indices, attribs, path);
        std::vector<std::unordered_map<size_t, u16>> node_to_joint;
        gltf_collect_skeletons(model, result.skeletons, node_to_joint, path);
        tinygltf::Scene& scene = model.scenes[model.defaultScene];
        gltf_collect_nodes(
            model, scene.nodes, Mat<4>(), 
            primitive_indices, result.meshes, 
            node_to_joint, result.skeletons,
            path
        );
        gltf_collect_animations(model, result.animations, node_to_joint, path);
        return result;
    }


    std::tuple<Mesh&, const Texture&, const Animation::Skeleton*> Model::mesh(
        const std::string& mesh_name
    ) {
        auto element = this->meshes.find(mesh_name);
        if(element == this->meshes.end()) {
           error("Model does not have mesh '" + mesh_name + "'");
        }
        Mesh& mesh = this->primitives.at(std::get<0>(element->second));
        const Texture& texture = this->textures.at(std::get<1>(element->second));
        std::optional<size_t> skeleton_id = std::get<2>(element->second);
        return {
            mesh, texture, 
            skeleton_id.has_value()
                ? &this->skeletons.at(*skeleton_id)
                : nullptr
        };
    }


    void Model::render_all(
        Shader& shader, const Texture& dest,
        std::optional<std::string_view> texture_uniform,
        bool depth_test
    ) {
        for(auto& [name, mesh]: this->meshes) {
            (void) name;
            if(texture_uniform.has_value()) {
                const Texture& texture = this->textures.at(std::get<1>(mesh));
                shader.set_uniform(*texture_uniform, texture);
            }
            Mesh& primitive = this->primitives.at(std::get<0>(mesh));
            primitive.render(shader, dest, depth_test);
        }
    }

}