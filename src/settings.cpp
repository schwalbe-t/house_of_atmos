
#include <nlohmann/json.hpp>
#include "settings.hpp"
#include "ui_const.hpp"
#include "audio_const.hpp"
#include <filesystem>
#include <fstream>
#include <format>

namespace houseofatmos {

    using json = nlohmann::json;

    Settings Settings::read_from_path(std::string_view path) {
        if(!std::filesystem::exists(path)) { return Settings(); }
        std::string text = engine::GenericLoader::read_string(path);
        json j = json::parse(text);
        auto s = Settings();
        if(j.contains("locale")) { s.locale = j.at("locale"); }
        if(j.contains("fullscreen")) { s.fullscreen = j.at("fullscreen"); }
        if(j.contains("music_volume")) { 
            s.ost_volume->gain = j.at("music_volume"); 
        }
        if(j.contains("sound_volume")) { 
            s.sfx_volume->gain = j.at("sound_volume"); 
        }
        if(j.contains("view_distance")) { 
            s.view_distance = j.at("view_distance"); 
        }
        if(j.contains("ui_size_divisor")) {
            s.ui_size_divisor = j.at("ui_size_divisor");
        }
        if(j.contains("last_games")) {
            for(const auto& path: j.at("last_games")) {
                s.last_games.push_back(path);
            }
        }
        return s;
    }

    void Settings::save_to(std::string_view path) const {
        json serialized = json::object();
        serialized["locale"] = this->locale;
        serialized["fullscreen"] = this->fullscreen;
        serialized["music_volume"] = this->ost_volume->gain;
        serialized["sound_volume"] = this->sfx_volume->gain;
        serialized["view_distance"] = this->view_distance;
        serialized["ui_size_divisor"] = this->ui_size_divisor;
        json last_games = json::array();
        for(const std::string& game: this->last_games) {
            last_games.push_back(game);
        }
        serialized["last_games"] = std::move(last_games);
        auto fout = std::ofstream(std::string(path));
        fout << serialized.dump(4);
        fout.close();
    }

    void Settings::add_recent_game(std::string&& path) {
        std::erase(this->last_games, path);
        this->last_games.insert(this->last_games.begin(), std::move(path));
    }

    ui::Element Settings::create_slider(
        f64 length_units, f64 height_units, f64 value_width_units,
        f64 min, f64 max, f64 step, f64 value,
        std::string suffix, std::function<void (f64)>&& handler
    ) {
        auto click_handler = [min, max, step, suffix, h = std::move(handler)](
            ui::Element& container, Vec<2> container_cursor
        ) {
            ui::Element& slider = container.child_at<0>();
            f64 cursor_x = container_cursor.x() - container.final_pos().x()
                + slider.final_pos().x();
            f64 n_value = std::min(
                std::max(cursor_x / slider.final_size().x(), 0.0), 1.0
            );
            f64 range = max - min;
            f64 r_value = round(n_value * range / step) * step;
            f64 f_value = min + r_value;
            ui::Element& handle = slider.child_at<1>();
            handle.position.x() = r_value / range;
            ui::Element& value = container.child_at<1>().child_at<0>();
            value.text = std::format("{}{}", f_value, suffix);
            h(f_value);
        };
        ui::Element container = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Horizontal)
            .with_click_handler(std::move(click_handler), ui::include_dragging)
            .as_movable();
        ui::Element slider = ui::Element()
            .as_phantom()
            .with_size(length_units, height_units, ui::size::units)
            .with_pos(0, 0, ui::position::parent_list_units)
            .as_movable();
        slider.children.push_back(ui::Element()
            .as_phantom()
            .with_size(length_units, 0.0, ui::size::units)
            .with_pos(0.5, 0.5, ui::position::parent_offset_fract)
            .with_background(&ui_background::border_dark)
            .as_movable()
        );
        f64 n_value = (value - min) / (max - min);
        slider.children.push_back(ui::Element()
            .with_size(2, height_units, ui::size::units)
            .with_pos(n_value, 0.5, ui::position::parent_offset_fract)
            .with_background(
                &ui_background::button, &ui_background::button_select
            )
            .as_movable()  
        );
        container.children.push_back(std::move(slider));
        f64 text_vert_pad = (height_units - ui_font::dark.height) / 2.0;
        ui::Element value_e = ui::Element()
            .with_size(value_width_units, height_units, ui::size::units)
            .with_pos(0, 0, ui::position::parent_list_units)
            .with_child(ui::Element()
                .with_pos(4.0, text_vert_pad, ui::position::parent_offset_units)
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(std::format("{}{}", value, suffix), &ui_font::dark)
                .as_movable()
            )
            .as_movable();
        container.children.push_back(std::move(value_e));
        return container;
    }

    static ui::Element create_text(std::string text) {
        ui::Element span = ui::Element()
            .as_phantom()
            .with_size(0, 0, ui::size::unwrapped_text)
            .with_text(text, &ui_font::dark)
            .with_padding(2.0)
            .as_phantom()
            .as_movable();
        return span;
    }

    static ui::Element create_button(
        std::string text, std::function<void (ui::Element&, Vec<2>)>&& handler
    ) {
        ui::Element button = ui::Element()
            .as_phantom()
            .with_size(0, 0, ui::size::unwrapped_text)
            .with_text(text, &ui_font::bright)
            .with_padding(2.0)
            .with_background(
                &ui_background::button, &ui_background::button_select
            )
            .with_click_handler(std::move(handler))
            .with_padding(2.0)
            .as_phantom()
            .as_movable();
        return button;
    }

    ui::Element Settings::create_menu(
        const engine::Localization& local, engine::Window& window,
        std::function<void ()>&& close_handler
    ) {
        window.cancel(engine::Button::Left);
        ui::Element menu = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_pos(0.5, 0.5, ui::position::window_fract)
            .with_background(&ui_background::scroll_vertical)
            .with_list_dir(ui::Direction::Vertical)
            .as_movable();
        ui::Element change_window_mode = create_button(
            this->fullscreen
                ? local.text("menu_windowed")
                : local.text("menu_fullscreen"),
            [this, local = &local](
                ui::Element& element, Vec<2> cursor
            ) {
                (void) cursor;
                this->fullscreen = !this->fullscreen;
                element.child_at<0>().text = this->fullscreen
                    ? local->text("menu_windowed")
                    : local->text("menu_fullscreen");
            }
        );
        menu.children.push_back(
            change_window_mode.with_padding(2.0).as_movable()
        );        
        menu.children.push_back(create_text(local.text("menu_music_volume")));
        ui::Element ost_volume = Settings::create_slider(
            96.0, 8.0, 24.0,
            0, 100, 5, this->ost_volume->gain * 100.0, 
            "%", [this](f64 percentage) {
                this->ost_volume->gain = percentage / 100.0; 
            }
        );
        menu.children.push_back(ost_volume.with_padding(4.0).as_movable());
        menu.children.push_back(create_text(local.text("menu_sound_volume")));
        ui::Element sfx_volume = Settings::create_slider(
            96.0, 8.0, 24.0,
            0, 100, 5, this->sfx_volume->gain * 100.0, 
            "%", [this](f64 percentage) { 
                this->sfx_volume->gain = percentage / 100.0; 
            }
        );
        menu.children.push_back(sfx_volume.with_padding(4.0).as_movable());
        menu.children.push_back(create_text(local.text("menu_view_distance")));
        ui::Element view_distance = Settings::create_slider(
            96.0, 8.0, 24.0,
            1, 5, 1, this->view_distance, 
            "", [this](f64 value) { 
                this->view_distance = (u64) value;
            }
        );
        menu.children.push_back(view_distance.with_padding(4.0).as_movable());
        menu.children.push_back(create_text(local.text("menu_ui_size")));
        // 1 => 550, 2 => 500, 3 => 450, 4 => 400, 
        // 5 => 350, 6 => 300, 7 => 250, 8 => 200
        ui::Element ui_size = Settings::create_slider(
            96.0, 8.0, 24.0,
            1, 8, 1, 
            (600.0 - this->ui_size_divisor) / 50.0, 
            "", [this, window = &window](f64 value) { 
                this->ui_size_divisor = 600.0 - (value * 50.0);
                window->cancel(engine::Button::Left);
            }
        );
        menu.children.push_back(ui_size.with_padding(4.0).as_movable());
        menu.children.push_back(create_button(
                local.text("menu_close_menu"),
                [h = std::move(close_handler)](ui::Element& e, Vec<2> c) {
                    (void) e; (void) c;
                    h();
                }
            )
            .with_padding(2.0)
            .as_movable()
        );
        return menu;
    }

    void Settings::apply(engine::Scene& scene, engine::Window& window) const {
        if(this->fullscreen && !window.is_fullscreen()) { 
            window.set_fullscreen(); 
        }
        if(!this->fullscreen && window.is_fullscreen()) {
            window.set_windowed();
        }
        scene.get(audio_const::soundtrack).speaker.volume = this->ost_volume;
        // other speakers contain references to their respective controlling volumes
        // view distance needs to be respected by the specific scene
        // ui size needs to be respected by the specific scene
    }

}