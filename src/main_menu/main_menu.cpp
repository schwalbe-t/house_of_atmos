
#include <samhocevar/portable-file-dialogs.h>
#include "main_menu.hpp"
#include "../outside/outside.hpp"
#include <filesystem>
#include <algorithm>
#include <thread>

namespace houseofatmos {

    MainMenu::MainMenu(Settings&& settings) {
        this->settings = std::move(settings);
        this->local_ref = this->settings.localization();
        u32 background_seed = random_init();
        this->terrain.generate_elevation(background_seed);
        this->terrain.generate_foliage(background_seed);
        this->renderer.camera.look_at = Vec<3>(16, 1, 16) 
            * this->terrain.units_per_tile();
        this->renderer.camera.position = this->renderer.camera.look_at
            + Vec<3>(0, 0, 35);
        this->renderer.camera.position.y()
            = this->terrain.elevation_at(this->renderer.camera.look_at) + 35;
        this->renderer.lights.push_back(outside::Outside::create_sun());
        this->load_resources();
    }

    void MainMenu::load_resources() {
        Renderer::load_shaders(*this);
        outside::Terrain::load_resources(*this);
        outside::Building::load_models(*this);
        outside::Foliage::load_models(*this);
        ui::Manager::load_shaders(*this);
        ui_const::load_all_textures(*this);
        this->load(engine::Localization::Loader(this->local_ref));
        this->load(engine::Texture::Loader(MainMenu::title_sprite.texture));
        this->load(engine::Shader::Loader(MainMenu::blur_shader));
    }

    static ui::Element make_button(
        const std::string& text, std::function<void ()>&& handler
    ) {
        ui::Element button = ui::Element()
            .as_phantom()
            .with_size(0, 0, ui::size::unwrapped_text)
            .with_text(text, &ui_font::bright)
            .with_padding(3)
            .with_background(
                &ui_background::button, &ui_background::button_select
            )
            .with_click_handler(std::move(handler))
            .with_padding(3)
            .as_movable();
        return button;
    }

    static void remove_missing_last_games(Settings& settings) {
        auto file_missing = [](const auto& path) { 
            return !std::filesystem::exists(path); 
        };
        std::erase_if(settings.last_games, file_missing);
        settings.save_to(Settings::default_path);
    }

    static void load_game_from(
        const std::string& path, engine::Window& window, Settings settings 
    ) {
        std::vector<char> data = engine::GenericResource::read_bytes(path);
        auto buffer = engine::Arena(data);
        auto game = std::make_shared<outside::Outside>(
            std::move(settings), buffer
        );
        game->save_path = path;
        game->settings.add_recent_game(std::string(path));
        game->settings.save_to(Settings::default_path);
        window.set_scene(std::move(game));
    }

    static const size_t max_prev_games = 5;

    void MainMenu::show_title_screen(
        const engine::Localization& local, engine::Window& window
    ) {
        std::vector<std::string>& last_games = this->settings.last_games;
        if(last_games.size() > max_prev_games) {
            last_games.erase(
                last_games.begin() + max_prev_games, last_games.end()
            );
        }
        remove_missing_last_games(this->settings);
        this->ui.root.children.clear();
        this->ui.with_element(ui::Element()
            .with_pos(0.5, 0.1, ui::position::window_fract)
            .with_size(
                MainMenu::title_sprite.edge_size.x(),
                MainMenu::title_sprite.edge_size.y(),
                ui::size::units
            )
            .with_background(&MainMenu::title_sprite)
            .as_movable()
        );
        ui::Element buttons = ui::Element()
            .with_pos(0.5, 0.8, ui::position::window_fract)
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Vertical)
            .as_movable();
        buttons.children.push_back(make_button(
            local.text("ui_new_game"),
            [window = &window, local = &local, this]() {
                this->before_next_frame = [window, this]() {
                    window->set_scene(std::make_shared<outside::Outside>(
                        std::move(settings)
                    ));
                };
                this->show_loading_screen(*local);
            }
        ));
        for(const std::string& game_path: this->settings.last_games) {
            size_t last_slash = game_path.find_last_of('/');
            size_t last_backslash = game_path.find_last_of('\\');
            size_t short_start = last_slash != std::string::npos
                ? last_slash + 1
                : last_backslash != std::string::npos
                    ? last_backslash + 1
                    : 0;
            std::string short_name = game_path.substr(short_start);
            buttons.children.push_back(make_button(
                local.pattern("ui_load_previous_game", { short_name }),
                [window = &window, local = &local, this, path = game_path]() {
                    this->before_next_frame = [window, this, path]() {
                        load_game_from(path, *window, this->settings);
                    };
                    this->show_loading_screen(*local);
                }
            ));
        }
        buttons.children.push_back(make_button(
            local.text("ui_load_game"),
            [local = &local, window = &window, this]() {
                std::vector<std::string> chosen = pfd::open_file(
                    local->text("ui_choose_load_location"),
                    "",
                    { local->text("ui_save_file"), "*.bin" }
                ).result();
                if(chosen.empty()) { return; }
                if(!std::filesystem::exists(chosen[0])) { return; }
                this->before_next_frame = [window, this, path = chosen[0]]() {
                    load_game_from(path, *window, this->settings);
                };
                this->show_loading_screen(*local);
            }
        ));
        buttons.children.push_back(make_button(
            local.text("ui_credits"),
            [this, local = &local, window = &window]() {
                this->show_credits(*local, *window);
            }
        ));
        buttons.children.push_back(make_button(
            local.text("ui_close_game"),
            []() { std::exit(0); }
        ));
        this->ui.with_element(buttons
            .with_padding(5)
            .with_background(&ui_background::scroll_vertical)
            .as_movable()
        );
        this->ui.with_element(ui::Element()
            .as_phantom()
            .with_pos(10, 10, ui::position::window_br_units)
            .with_size(
                ui_icon::earth.edge_size.x(), ui_icon::earth.edge_size.y(), 
                ui::size::units
            )
            .with_background(&ui_icon::earth)
            .with_padding(1)
            .with_background(
                &ui_background::border, &ui_background::border_hovering
            )
            .with_click_handler([this, window = &window]() {
                this->show_language_selection(*window);
            })
            .with_padding(2)
            .with_background(&ui_background::note)
            .as_movable()
        );
        this->ui.with_element(ui::Element()
            .with_pos(7.5, 7.5, ui::position::window_bl_units)
            .with_size(0, 0, ui::size::unwrapped_text)
            .with_text(local.text("ui_copyright_notice"), &ui_font::bright)
            .as_movable()
        );
    }

    void MainMenu::show_loading_screen(const engine::Localization& local) {
        this->ui.root.children.clear();
        this->ui.with_element(ui::Element()
            .with_pos(0.5, 0.5, ui::position::window_fract)
            .with_size(0, 0, ui::size::unwrapped_text)
            .with_text(local.text("ui_loading"), &ui_font::bright)
            .as_movable()
        );
    }

    static const std::vector<std::pair<std::string, std::string>> languages = {
        { "English", "en" },
        { "Deutsch", "de" },
        { "Български", "bg" }
    };

    void MainMenu::show_language_selection(engine::Window& window) {
        this->ui.root.children.clear();
        ui::Element selection = ui::Element()
            .with_pos(0.5, 0.5, ui::position::window_fract)
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Vertical)
            .as_movable();
        for(const auto& [name, locale]: languages) {
            selection.children.push_back(make_button(
                name, 
                [this, locale = &locale, window = &window]() {
                    this->settings.locale = *locale;
                    this->settings.save_to(Settings::default_path);
                    window->set_scene(std::make_shared<MainMenu>(
                        Settings(this->settings)
                    ));
                }
            ));
        }
        this->ui.with_element(selection
            .with_padding(5)
            .with_background(&ui_background::scroll_vertical)
            .as_movable()
        );
    }

    void MainMenu::show_credits(
        const engine::Localization& local, engine::Window& window
    ) {
        this->ui.root.children.clear();
        this->ui.root.children.push_back(ui::Element()
            .with_pos(0.5, 0.15, ui::position::window_fract)
            .with_size(0, 0, ui::size::unwrapped_text)
            .with_text(local.text("ui_credits"), &ui_font::bright)
            .as_movable()
        );
        ui::Element credits = ui::Element()
            .with_pos(0.5, 0.7, ui::position::window_fract)
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Vertical)
            .as_movable();
        std::string credit_text = local.text("ui_credit_text");
        size_t start = 0;
        while(start < credit_text.size()) {
            size_t end = credit_text.find('\n', start);
            if(end == std::string::npos) { end = credit_text.size(); }
            std::string line = credit_text.substr(start, end - start);
            credits.children.push_back(ui::Element()
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(line, &ui_font::dark)
                .with_padding(1)
                .as_movable()
            );
            start = end + 1;
        }
        this->ui.root.children.push_back(credits
            .with_padding(4)
            .with_background(&ui_background::scroll_vertical)
            .as_movable()
        );
        auto close_credits = [this, local = &local, window = &window]() {
            this->show_title_screen(*local, *window);
        };
        this->ui.root.children.push_back(make_button("X", std::move(close_credits))
            .with_pos(25.0, 25.0, ui::position::window_tl_units)
            .as_movable()
        );
    }

    void MainMenu::update(engine::Window& window) {
        this->before_next_frame();
        this->before_next_frame = []() {};
        if(this->ui.root.children.size() == 0) {
            const auto& local = this->get<engine::Localization>(this->local_ref);
            if(this->settings.locale == engine::Localization::no_locale) {
                this->show_language_selection(window);
            } else {
                this->show_title_screen(local, window); 
            }
        }
        this->ui.update(window);
    }

    void MainMenu::render_background(engine::Window& window) {
        this->renderer.configure(window, *this);
        this->terrain.load_chunks_around(this->renderer.camera.look_at);
        this->renderer.render_to_shadow_maps();
        this->terrain.render_loaded_chunks(*this, this->renderer, window);
        this->renderer.render_to_output();
        this->terrain.render_loaded_chunks(*this, this->renderer, window);
        this->background.resize_fast(
            this->renderer.output().width(), this->renderer.output().height()
        );
        engine::Shader& blur = this->get<engine::Shader>(MainMenu::blur_shader);
        blur.set_uniform("u_texture_w", (i64) this->background.width());
        blur.set_uniform("u_texture_h", (i64) this->background.height());
        i64 blur_rad = (i64) ((window.width() + window.height()) / 200.0);
        blur.set_uniform("u_blur_rad", blur_rad);
        this->renderer.output().blit(this->background.as_target(), blur);
    }

    void MainMenu::render(engine::Window& window) {
        this->render_background(window);
        window.show_texture(this->background);
        this->ui.render(*this, window);
        window.show_texture(this->ui.output());
    }

}