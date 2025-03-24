
#pragma once

#include <engine/scene.hpp>
#include <engine/localization.hpp>
#include <engine/ui.hpp>

namespace houseofatmos {

    namespace ui = houseofatmos::engine::ui;
    using namespace houseofatmos::engine::math;


    struct Settings {
        
        static const inline std::string_view default_path = "settings.json";


        std::string locale = std::string(engine::Localization::no_locale);
        bool fullscreen = false;
        std::shared_ptr<engine::Volume> ost_volume 
            = std::make_shared<engine::Volume>(engine::Volume(1.0));
        std::shared_ptr<engine::Volume> sfx_volume
            = std::make_shared<engine::Volume>(engine::Volume(1.0));
        u64 view_distance = 2;
        f64 ui_size_divisor = 300.0;
        std::vector<std::string> last_games;

        Settings() {}
        static Settings read_from_path(std::string_view path);
        void save_to(std::string_view path) const;

        void add_recent_game(std::string&& path);

        engine::Localization::LoadArgs localization() const {
            return engine::Localization::LoadArgs(
                "res/localization.json", this->locale
            );
        }

        f64 ui_size_fract() const {
            return 1.0 / this->ui_size_divisor;
        }

        static ui::Element create_slider(
            f64 length_units, f64 height_units, f64 value_width_units, 
            f64 min, f64 max, f64 step, f64 value,
            std::string suffix, std::function<void (f64)>&& handler
        );
        ui::Element create_menu(
            const engine::Localization& local, engine::Window& window,
            std::function<void ()>&& close_handler
        );

        void apply(engine::Scene& scene, engine::Window& window) const;

    };

}