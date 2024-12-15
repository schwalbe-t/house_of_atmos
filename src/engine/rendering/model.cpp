
#include <engine/rendering.hpp>
#include <engine/logging.hpp>
#include <nlohmann/json.hpp>
#define TINYGLTF_NO_INCLUDE_JSON
#include <stb/stb_image.h>
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <syoyo/tiny_gltf.h>

namespace houseofatmos::engine {

    Model::Model() {}

    Model Model::from_resource(const Model::LoadArgs& args) {
        const std::string& path = args.path;
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
        // TODO! loading of values 
        return Model();
    }

}