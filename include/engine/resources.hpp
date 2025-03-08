
#pragma once

#include <vector>
#include <string>
#include <optional>

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

    template<typename R, typename A>
    struct ResourceLoader: GenericLoader {

        private:
        std::optional<R> loaded_value;
        A args;


        public:
        constexpr ResourceLoader(A args): args(std::move(args)) {
            this->loaded_value = std::nullopt;
        }
        ResourceLoader(ResourceLoader&& other): args(std::move(other.args)) {
            this->has_value = other.has_value;
            this->loaded_value = std::move(other.loaded_value);
        }
        ~ResourceLoader() override = default;


        void load() override {
            this->has_value = true;
            this->loaded_value = R::from_resource(this->args);
        }
        const A& arg() const { return this->args; }
        R& value() { return this->loaded_value.value(); }
    };

}