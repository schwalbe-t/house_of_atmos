
#pragma once

#include <engine/scene.hpp>
#include <engine/localization.hpp>

namespace houseofatmos {

    struct Settings {
        
        static const inline std::string default_path = "settings.json";

        struct LoadArgs {
            static LoadArgs from_default() {
                return (LoadArgs) { Settings::default_path };
            }

            std::string path;

            std::string identifier() const { return path; }
            std::string pretty_identifier() const {
                return "Settings@'" + path + "'"; 
            }
        };
        using Loader = engine::Resource<Settings, LoadArgs>;


        std::string locale = "en";
        std::vector<std::string> last_games;

        Settings() {}
        static Settings from_resource(const LoadArgs& args);
        void save_to(const std::string& path) const;

        engine::Localization::LoadArgs localization() const {
            return (engine::Localization::LoadArgs) {
                "res/localization.json", this->locale 
            };
        }

    };

}