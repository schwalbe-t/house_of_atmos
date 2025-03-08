
#pragma once

#include "nums.hpp"
#include "logging.hpp"
#include <unordered_map>
#include <optional>
#include <vector>
#include <string>
#include <memory>

namespace houseofatmos::engine {

    struct GenericLoader {

        protected:
        bool has_value = false;


        public:
        GenericLoader& operator=(GenericLoader&& other) = default;
        virtual ~GenericLoader() = default;

        virtual void load() = 0;
        bool loaded() const { return this->has_value; }
    
        static std::vector<char> read_bytes(std::string_view path);
        static std::string read_string(std::string_view path);
    };

    template<typename T, typename A>
    struct ResourceLoader: GenericLoader {

        private:
        std::optional<T> loaded_value;
        A args;


        public:
        constexpr ResourceLoader(A arg) {
            this->args = arg;
            this->loaded_value = std::nullopt;
        }
        ResourceLoader(ResourceLoader&& other) {
            this->has_value = other.has_value;
            this->args = std::move(other.args);
            this->loaded_value = std::move(other.loaded_value);
        }
        ~ResourceLoader() override = default;


        void load() override {
            this->has_value = true;
            this->loaded_value = T::from_resource(this->args);
        }
        const A& arg() const { return this->args; }
        T& value() { return this->loaded_value.value(); }
    };

    struct Window;

    struct Scene {

        private:
        std::unordered_map<std::string, std::shared_ptr<GenericLoader>> resources;


        public:
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
            using T = typename A::ResourceType;
            auto res = ResourceLoader<T, A>(std::move(arg));
            std::string iden = res.arg().identifier();
            std::shared_ptr<GenericLoader> cached 
                = Scene::get_cached_resource(iden);
            if(cached == nullptr) {
                cached = std::make_shared<ResourceLoader<T, A>>(std::move(res));
                Scene::put_cached_resource(iden, cached);
            }
            this->resources[iden] = std::move(cached);
        }

        template<typename A>
        auto& get(const A& arg) {
            using T = typename A::ResourceType;
            std::string iden = arg.identifier();
            auto res_ref = this->resources.find(iden);
            if(res_ref == this->resources.end()) {
                error("Resource "
                    + arg.pretty_identifier()
                    + " has not been registered, but access was attempted"
                );
            }
            GenericLoader* dr = res_ref->second.get();
            auto res = dynamic_cast<ResourceLoader<T, A>*>(dr);
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