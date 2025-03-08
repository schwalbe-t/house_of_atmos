
#pragma once

#include "nums.hpp"
#include "logging.hpp"
#include "audio.hpp"
#include "resources.hpp"
#include <unordered_map>
#include <optional>
#include <vector>
#include <string>
#include <memory>

namespace houseofatmos::engine {

    struct Window;

    struct Scene {

        private:
        std::unordered_map<std::string, std::shared_ptr<GenericLoader>> resources;


        public:
        Listener listener;

        Scene();
        Scene(const Scene& other) = delete;
        Scene(Scene&& other) = delete;
        Scene& operator=(const Scene& other) = delete;
        Scene& operator=(Scene&& other) = delete;
        virtual ~Scene() = default;

        static std::shared_ptr<GenericLoader> get_cached_resource(
            const std::string& iden
        );

        static void put_cached_resource(
            std::string iden, std::shared_ptr<GenericLoader>& resource
        );

        static void clean_cached_resources();

        template<typename A>
        void load(A arg) {
            using R = typename A::ResourceType;
            std::string iden = arg.identifier();
            std::shared_ptr<GenericLoader> cached 
                = Scene::get_cached_resource(iden);
            if(cached == nullptr) {
                cached = std::make_shared<ResourceLoader<R, A>>(std::move(arg));
                Scene::put_cached_resource(iden, cached);
            }
            this->resources[iden] = std::move(cached);
        }

        template<typename A>
        auto& get(const A& arg) {
            using R = typename A::ResourceType;
            std::string iden = arg.identifier();
            auto res_ref = this->resources.find(iden);
            if(res_ref == this->resources.end()) {
                error("Resource "
                    + arg.pretty_identifier()
                    + " has not been registered, but access was attempted"
                );
            }
            GenericLoader* dr = res_ref->second.get();
            auto res = dynamic_cast<ResourceLoader<R, A>*>(dr);
            if(!res->loaded()) {
                error("Resource "
                    + arg.pretty_identifier()
                    + " has not yet been loaded, but access was attempted"
                );
            }
            return res->value();
        }

        void internal_load_all();

        virtual void update(Window& window) = 0;
        virtual void render(Window& window) = 0;

    };

}