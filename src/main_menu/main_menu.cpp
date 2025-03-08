
#include <samhocevar/portable-file-dialogs.h>
#include "main_menu.hpp"
#include "../world/scene.hpp"
#include "../tutorial/tutorial.hpp"
#include <filesystem>
#include <algorithm>

namespace houseofatmos {

    MainMenu::MainMenu(Settings&& settings):
            settings(std::move(settings)), local_ref(settings.localization()) {
        u32 background_seed = random_init();
        this->terrain.generate_elevation(background_seed);
        this->terrain.generate_foliage(background_seed);
        this->renderer.camera.look_at = Vec<3>(16, 0.0, 16) 
            * this->terrain.units_per_tile();
        this->renderer.camera.look_at
            .y() = this->terrain.elevation_at(this->renderer.camera.look_at);
        this->renderer.fog_origin = this->renderer.camera.look_at;
        this->renderer.lights.push_back(
            world::Scene::create_sun(this->renderer.camera.look_at)
        );
        this->load_resources();
    }

    void MainMenu::load_resources() {
        Renderer::load_shaders(*this);
        world::Terrain::load_resources(*this);
        world::Foliage::load_models(*this);
        ui::Manager::load_shaders(*this);
        ui_const::load_all(*this);
        audio_const::load_all(*this);
        this->load(this->local_ref);
        this->load(MainMenu::title_sprite.texture);
        this->load(MainMenu::blur_shader);
    }

    static ui::Element make_button(
        const std::string& text, std::function<void ()>&& handler
    ) {
        ui::Element button = ui::Element()
            .as_phantom()
            .with_size(0, 0, ui::size::unwrapped_text)
            .with_text(text, &ui_font::bright)
            .with_padding(2)
            .with_background(
                &ui_background::button, &ui_background::button_select
            )
            .with_click_handler(std::move(handler))
            .with_padding(2)
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
        std::vector<char> data = engine::GenericLoader::read_bytes(path);
        auto buffer = engine::Arena(data);
        auto world = std::make_shared<world::World>(std::move(settings), buffer);
        world->save_path = path;
        world->settings.add_recent_game(std::string(path));
        world->settings.save_to(Settings::default_path);
        window.set_scene(std::make_shared<world::Scene>(std::move(world)));
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
            local.text("menu_new_game"),
            [window = &window, local = &local, this]() {
                this->before_next_frame = [window, this]() {
                    auto world_after = std::make_shared<world::World>(
                        Settings(this->settings), 256, 256
                    );
                    world_after->generate_map(random_init());
                    window->set_scene(tutorial::create_toddler_scene(
                        std::move(world_after)
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
                local.pattern("menu_load_previous_game", { short_name }),
                [window = &window, local = &local, this, path = game_path]() {
                    this->before_next_frame = [window, this, path]() {
                        load_game_from(path, *window, this->settings);
                    };
                    this->show_loading_screen(*local);
                }
            ));
        }
        buttons.children.push_back(make_button(
            local.text("menu_load_game"),
            [local = &local, window = &window, this]() {
                std::vector<std::string> chosen = pfd::open_file(
                    local->text("menu_choose_load_location"),
                    "",
                    { local->text("menu_save_file"), "*.bin" }
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
            local.text("menu_settings"),
            [this, local = &local, window = &window]() {
                this->show_settings(*local, *window);
            }
        ));
        buttons.children.push_back(make_button(
            local.text("menu_exit_game"),
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
            .with_text(local.text("menu_copyright_notice"), &ui_font::bright)
            .as_movable()
        );
    }

    void MainMenu::show_loading_screen(const engine::Localization& local) {
        this->ui.root.children.clear();
        this->ui.with_element(ui::Element()
            .with_pos(0.5, 0.5, ui::position::window_fract)
            .with_size(0, 0, ui::size::unwrapped_text)
            .with_text(local.text("menu_loading"), &ui_font::bright)
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

    void MainMenu::show_settings(
        const engine::Localization& local, engine::Window& window
    ) {
        this->ui.root.children.clear();
        this->ui.with_element(this->settings.create_menu(
            local,
            [this, local = &local, window = &window]() {
                this->settings.save_to(Settings::default_path);
                this->show_title_screen(*local, *window);
            }
        ));
    }

    static const f64 camera_rotation_period = 60.0;
    static const f64 camera_y = 1.0; // 45 degrees
    static const f64 camera_distance = 40.0;

    void MainMenu::update(engine::Window& window) {
        this->settings.apply(*this, window);
        this->get(audio_const::soundtrack).update();
        this->before_next_frame();
        this->before_next_frame = []() {};
        if(this->ui.root.children.size() == 0) {
            const auto& local = this->get(this->local_ref);
            if(this->settings.locale == engine::Localization::no_locale) {
                this->show_language_selection(window);
            } else {
                this->show_title_screen(local, window); 
            }
        }
        this->ui.update(window);
        f64 cam_angle = window.time() / camera_rotation_period * (2 * pi);
        this->renderer.camera.position = this->renderer.camera.look_at
            + Vec<3>(cos(cam_angle), camera_y, sin(cam_angle)) * camera_distance;
    }

    void MainMenu::render_background(engine::Window& window) {
        world::Scene::configure_renderer(
            this->renderer, this->settings.view_distance
        );
        this->renderer.configure(window, *this);
        this->terrain.load_chunks_around(
            this->renderer.camera.look_at, 
            MainMenu::draw_distance_ch, nullptr, window, nullptr
        );
        this->renderer.render_to_shadow_maps();
        this->terrain.render_loaded_chunks(*this, this->renderer, window);
        this->renderer.render_to_output();
        this->terrain.render_loaded_chunks(*this, this->renderer, window);
        this->terrain.render_water(*this, this->renderer, window);
        this->background.resize_fast(
            this->renderer.output().width(), this->renderer.output().height()
        );
        engine::Shader& blur = this->get(MainMenu::blur_shader);
        blur.set_uniform("u_texture_w", (i64) this->background.width());
        blur.set_uniform("u_texture_h", (i64) this->background.height());
        i64 blur_rad = (i64) (std::min(window.width(), window.height()) / 100.0);
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