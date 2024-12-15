
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
        bool has_value;


        public:
        GenericResource& operator=(GenericResource&& other) = default;
        virtual ~GenericResource() = default;

        virtual void load() = 0;
        virtual void forget() = 0;
        bool loaded() const { return this->has_value; }
    
        static std::vector<char> read_bytes(const std::string& path);
        static std::string read_string(const std::string& path);
    };

    template<typename T, typename A>
    struct Resource: GenericResource {

        private:
        std::optional<T> loaded_value;
        A args;


        public:
        constexpr Resource(A arg) {
            this->args = arg;
            this->loaded_value = std::nullopt;
        }
        Resource& operator=(Resource&& other) = default;
        ~Resource() override = default;


        void load() override {
            this->has_value = true;
            this->loaded_value = T::from_resource(this->args);
        }
        void forget() override {
            this->has_value = false;
            this->loaded_value = std::nullopt;
        }
        const A& arg() const { return this->args; }
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

        template<typename T, typename A>
        void load(Resource<T, A>&& resource) {
            Resource<T, A>* hr = new Resource<T, A>(std::move(resource));
            this->resources[hr->arg().identifier()] = hr;
        }
        template<typename T, typename A>
        T& get(const A& arg) {
            std::string identifier = arg.identifier();
            if(!this->resources.contains(identifier)) {
                error("Resource "
                    + arg.pretty_identifier()
                    + " has not been registered, but access was attempted"
                );
            }
            const GenericResource* dr = this->resources[identifier];
            auto r = dynamic_cast<const Resource<T, A>*>(dr);
            if(!r->loaded()) {
                error("Resource "
                    + arg.pretty_identifier()
                    + " has not yet been loaded, but access was attempted"
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