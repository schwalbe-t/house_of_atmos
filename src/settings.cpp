
#include <nlohmann/json.hpp>
#include "settings.hpp"
#include <filesystem>
#include <fstream>

namespace houseofatmos {

    using json = nlohmann::json;

    Settings Settings::read_from_path(const std::string& path) {
        if(!std::filesystem::exists(path)) { return Settings(); }
        std::string text = engine::GenericResource::read_string(path);
        json json = json::parse(text);
        Settings settings;
        settings.locale = json.at("locale");
        for(const auto& path: json.at("last_games")) {
            settings.last_games.push_back(path);
        }
        return settings;
    }

    void Settings::save_to(const std::string& path) const {
        json serialized = json::object();
        serialized["locale"] = this->locale;
        json last_games = json::array();
        for(const std::string& game: this->last_games) {
            last_games.push_back(game);
        }
        serialized["last_games"] = std::move(last_games);
        auto fout = std::ofstream(path);
        fout << serialized.dump(4);
        fout.close();
    }

    void Settings::add_recent_game(std::string&& path) {
        std::erase(this->last_games, path);
        this->last_games.insert(this->last_games.begin(), std::move(path));
    }

}