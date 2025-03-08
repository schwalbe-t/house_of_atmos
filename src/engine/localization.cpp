
#include <engine/localization.hpp>
#include <engine/logging.hpp>
#include <nlohmann/json.hpp>

namespace houseofatmos::engine {

    using json = nlohmann::json;


    Localization Localization::from_resource(const LoadArgs& args) {
        std::string raw = GenericLoader::read_string(args.path);
        json parsed = json::parse(raw);
        Localization result;
        result.locale = args.locale;
        if(args.locale == Localization::no_locale) {
            return result;
        }
        for(auto& [name, values]: parsed.items()) {
            if(name.size() == 0) { continue; }
            auto value = values.find(args.locale);
            if(value != values.end()) {
                result.values[name] = (std::string) *value;
                continue;
            }
            warning("Localization for locale '" 
                + std::string(args.locale)
                + "' does not include a value under the name '"
                + name + "'!"
            );
            auto fallback = values.find(args.fallback);
            if(fallback != values.end()) {
                result.values[name] = (std::string) *fallback;
            }
        }
        return result;
    }

}