
#pragma once

#include "scene.hpp"

namespace houseofatmos::engine {

    struct Localization {
        struct LoadArgs {
            std::string path;
            std::string locale;
            
            std::string identifier() const { return path + "|||" + locale; }
            std::string pretty_identifier() const {
                return "Localization[" + locale + "]@'" + path + "'"; 
            }
        };
        using Loader = Resource<Localization, LoadArgs>;


        private:
        std::string locale;
        std::unordered_map<std::string, std::string> values;

        Localization() {}


        public:
        static Localization from_resource(const LoadArgs& args);

        static inline const std::string missing_local 
            = "[missing localization; check console]";

        const std::string& text(const std::string& name) {
            auto value = this->values.find(name);
            if(value == this->values.end()) { return missing_local; }
            return value->second;
        }

    };

}