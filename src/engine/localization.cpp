
#include <engine/localization.hpp>
#include <nlohmann/json.hpp>

namespace houseofatmos::engine {

    using json = nlohmann::json;


    Localization Localization::from_resource(const LoadArgs& args) {
        std::string raw = GenericResource::read_string(args.path);
        json parsed = json::parse(raw);
        Localization result;
        result.locale = args.locale;
        if(args.locale == Localization::no_locale) {
            return result;
        }
        for(auto& [name, values]: parsed.items()) {
            if(name.size() == 0) { continue; }
            auto value = values.find(args.locale);
            if(value == values.end()) {
                engine::warning("Localization for locale '" 
                    + std::string(args.locale)
                    + "' does not include a value under the name '"
                    + name + "'!"
                );
                continue;
            }
            result.values[name] = (std::string) *value;
        }
        return result;
    }

}