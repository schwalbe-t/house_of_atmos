
#include <engine/scene.hpp>
#include <engine/logging.hpp>
#include <iostream>
#include <fstream>
#include <algorithm>

namespace houseofatmos::engine {

    std::vector<char> GenericResource::read_bytes(std::string_view path_v) {
        auto path = std::string(path_v);
        auto stream = std::ifstream(path, std::ios::binary);
        if(stream.fail()) {
            error("The file '" + path + "' could not be read");
        }
        auto iter = std::istreambuf_iterator<char>(stream);
        return std::vector(iter, std::istreambuf_iterator<char>());
    }

    std::string GenericResource::read_string(std::string_view path_v) {
        auto path = std::string(path_v);
        auto stream = std::ifstream(path, std::ios::binary);
        if(stream.fail()) {
            error("The file '" + path + "' could not be read");
        }
        auto iter = std::istreambuf_iterator<char>(stream);
        return std::string(iter, std::istreambuf_iterator<char>());
    }


    Scene::Scene() {}


    static std::unordered_map<std::string, std::weak_ptr<GenericResource>> cached;

    std::shared_ptr<GenericResource> Scene::get_cached_resource(
        const std::string& iden
    ) {
        auto res_ref = cached.find(iden);
        if(res_ref == cached.end()) { return nullptr; }
        std::shared_ptr<GenericResource> res = res_ref->second.lock();
        return res;
    }

    void Scene::put_cached_resource(
        std::string iden, std::shared_ptr<GenericResource>& resource
    ) {
        cached[std::move(iden)] = resource;
    }

    void Scene::clean_cached_resources() {
        auto is_expired = [](const auto& entry) {
            return entry.second.expired();
        };
        std::erase_if(cached, is_expired);
    }

    void Scene::internal_load_all() {
        for(auto& entry: this->resources) {
            GenericResource& res = *entry.second;
            if(!res.loaded()) { res.load(); }
        }
    }

}