
#pragma once

#include "scene.hpp"
#include <span>
#include <initializer_list>

namespace houseofatmos::engine {

    struct Localization {
        
        static inline const std::string no_locale = "";

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

        const std::string& text(const std::string& name) const {
            auto value = this->values.find(name);
            if(value == this->values.end()) { return missing_local; }
            return value->second;
        }

        static inline const std::string placeholder = "{}";

        const std::string pattern(
            const std::string& name, std::span<const std::string> values
        ) const {
            auto pattern_pos = this->values.find(name);
            if(pattern_pos == this->values.end()) { return missing_local; }
            std::string result = pattern_pos->second;
            size_t ph_idx;
            size_t value_i = 0;
            for(;;) {
                ph_idx = result.find(placeholder);
                if(ph_idx == std::string::npos) { break; }
                if(value_i >= values.size()) { break; }
                result.replace(ph_idx, placeholder.size(), values[value_i]);
                value_i += 1;
            }
            return result;
        }

        const std::string pattern(
            const std::string& name, std::initializer_list<const std::string> values
        ) const {
            return this->pattern(name, std::span(values));
        }
    };

}