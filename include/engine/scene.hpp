
#pragma once

#include "nums.hpp"
#include "logging.hpp"
#include <unordered_map>
#include <optional>
#include <vector>
#include <string>
#include <memory>

namespace houseofatmos::engine {

    struct GenericResource {

        protected:
        bool has_value = false;


        public:
        GenericResource& operator=(GenericResource&& other) = default;
        virtual ~GenericResource() = default;

        virtual void load() = 0;
        bool loaded() const { return this->has_value; }
    
        static std::vector<char> read_bytes(std::string_view path);
        static std::string read_string(std::string_view path);
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
        Resource(Resource&& other) {
            this->has_value = other.has_value;
            this->args = std::move(other.args);
            this->loaded_value = std::move(other.loaded_value);
        }
        ~Resource() override = default;


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
        std::unordered_map<std::string, std::shared_ptr<GenericResource>> resources;


        public:
        Scene();
        Scene(const Scene& other) = delete;
        Scene(Scene&& other) = delete;
        Scene& operator=(const Scene& other) = delete;
        Scene& operator=(Scene&& other) = delete;
        virtual ~Scene() = default;

        static std::shared_ptr<GenericResource> get_cached_resource(
            const std::string& iden
        );

        static void put_cached_resource(
            std::string iden, std::shared_ptr<GenericResource>& resource
        );

        static void clean_cached_resources();

        template<typename T, typename A>
        void load(Resource<T, A>&& res) {
            std::string iden = res.arg().identifier();
            std::shared_ptr<GenericResource> generic_res 
                = Scene::get_cached_resource(iden);
            if(generic_res == nullptr) {
                generic_res = std::make_shared<Resource<T, A>>(std::move(res));
                Scene::put_cached_resource(iden, generic_res);
            }
            this->resources[iden] = std::move(generic_res);
        }

        template<typename T, typename A>
        T& get(const A& arg) {
            std::string iden = arg.identifier();
            auto res_ref = this->resources.find(iden);
            if(res_ref == this->resources.end()) {
                error("Resource "
                    + arg.pretty_identifier()
                    + " has not been registered, but access was attempted"
                );
            }
            GenericResource* dr = res_ref->second.get();
            auto res = dynamic_cast<Resource<T, A>*>(dr);
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