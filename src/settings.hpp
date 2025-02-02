
#pragma once

#include <engine/scene.hpp>
#include <engine/localization.hpp>

namespace houseofatmos {

    struct Settings {
        
        static const inline std::string default_path = "settings.json";


        std::string locale = engine::Localization::no_locale;
        std::vector<std::string> last_games;

        Settings() {}
        static Settings read_from_path(const std::string& path);
        void save_to(const std::string& path) const;

        void add_recent_game(std::string&& path);

        engine::Localization::LoadArgs localization() const {
            return (engine::Localization::LoadArgs) {
                "res/localization.json", this->locale 
            };
        }

    };

}