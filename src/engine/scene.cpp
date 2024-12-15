
#include <engine/scene.hpp>
#include <engine/logging.hpp>
#include <iostream>
#include <fstream>

namespace houseofatmos::engine {

    std::vector<char> GenericResource::read_bytes(const std::string& path) {
        auto stream = std::ifstream(path);
        if(stream.fail()) {
            error("The file '" + path + "' could not be read");
        }
        auto iter = std::istreambuf_iterator<char>(stream);
        return std::vector(iter, std::istreambuf_iterator<char>());
    }

    std::string GenericResource::read_string(const std::string& path) {
        auto stream = std::ifstream(path);
        if(stream.fail()) {
            error("The file '" + path + "' could not be read");
        }
        auto iter = std::istreambuf_iterator<char>(stream);
        return std::string(iter, std::istreambuf_iterator<char>());
    }


    Scene::Scene() {}

    static void free_resources(
        std::unordered_map<std::string, GenericResource*>& resources
    ) {
        for(auto& entry: resources) {
            GenericResource* res = entry.second;
            delete res;
        }
    }

    Scene::~Scene() {
        free_resources(this->resources);
    }


    void Scene::internal_load_all() {
        for(auto& entry: this->resources) {
            GenericResource* res = entry.second;
            if(!res->loaded()) { res->load(); }
        }
    }

    void Scene::internal_forget_all() {
        for(auto& entry: this->resources) {
            GenericResource* res = entry.second;
            if(res->loaded()) { res->forget(); }
        }
    }

}