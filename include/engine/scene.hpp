
#pragma once

#include "nums.hpp"
#include "logging.hpp"
#include <unordered_map>
#include <optional>
#include <vector>
#include <string>

namespace houseofatmos::engine {

    struct GenericResource {

        protected:
        std::string path;
        bool has_value;


        public:
        virtual ~GenericResource() = default;

        virtual void load() = 0;
        virtual void forget() = 0;
        bool loaded() const { return this->has_value; }
        const std::string& source() { return this->path; }
    
        static std::vector<char> read_bytes(const std::string& path);
    };

    template<typename T>
    struct Resource: GenericResource {

        private:
        std::optional<T> loaded_value;


        public:
        Resource(std::string_view path) {
            this->path = std::string(path);
            this->loaded_value = std::nullopt;
        }
        Resource& operator=(Resource&& other) = default;
        ~Resource() override = default;


        void load() override {
            this->has_value = true;
            this->loaded_value = T(this->path);
        }
        void forget() override {
            this->has_value = false;
            this->loaded_value = std::nullopt;
        }
        const T& value() const { return this->loaded_value.value(); }
    };

    struct Window;

    struct Scene {

        private:
        std::unordered_map<std::string, GenericResource*> resources;


        public:
        Scene();
        Scene(const Scene& other) = delete;
        Scene(Scene&& other) = delete;
        Scene& operator=(const Scene& other) = delete;
        Scene& operator=(Scene&& other) = delete;
        virtual ~Scene();

        template<typename T>
        void load(Resource<T>&& resource) {
            Resource<T>* hr = new Resource<T>(resource.source());
            *hr = std::move(resource);
            this->resources[resource.source()] = hr;
        }
        template<typename T>
        const T& get(std::string_view path) {
            auto path_s = std::string(path);
            if(!this->resources.contains(path_s)) {
                error("Resource '"
                    + path_s
                    + "' has not been registered, but access was attempted"
                );
            }
            const GenericResource* dr = this->resources[path_s];
            auto r = dynamic_cast<const Resource<T>*>(dr);
            if(!r->loaded()) {
                error("Resource '"
                    + path_s
                    + "' has not yet been loaded, but access was attempted"
                );
            }
            return r->value();
        }

        void internal_load_all();
        void internal_forget_all();

        virtual void update(const Window& window) = 0;
        virtual void render(const Window& window) = 0;

    };

}