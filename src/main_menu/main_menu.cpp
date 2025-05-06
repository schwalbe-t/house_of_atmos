
#include <samhocevar/portable-file-dialogs.h>
#include "main_menu.hpp"
#include "../ui_util.hpp"
#include "../world/scene.hpp"
#include "../tutorial/tutorial.hpp"
#include <filesystem>
#include <algorithm>
#ifdef __EMSCRIPTEN__
    #include <emscripten.h>

    EM_JS(void, hoa_read_file, (void* raw_menu, void* raw_window, const void* raw_local), {
        const input = document.createElement("input");
        input.type = "file";
        input.onchange = e => {
            const file = e.target.files[0];
            const reader = new FileReader();
            reader.onload = () => {
                const arrayBuffer = reader.result;
                const byteArray = new Uint8Array(arrayBuffer);
                const size = byteArray.length;
                const ptr = _malloc(size);
                HEAPU8.set(byteArray, ptr);
                _hoa_handle_read_file(ptr, size, raw_menu, raw_window, raw_local);
            };
            reader.readAsArrayBuffer(file);
        };
        input.click();
    });

    extern "C" EMSCRIPTEN_KEEPALIVE void hoa_handle_read_file(
        uint8_t* data, int size, void* raw_menu, void* raw_window, const void* raw_local
    ) {
        auto menu = (houseofatmos::MainMenu*) raw_menu;
        auto window = (houseofatmos::engine::Window*) raw_window;
        auto local = (const houseofatmos::engine::Localization*) raw_local;
        std::span<const char> file_data((const char*) data, size);
        menu->load_game(file_data, "<download>", *local, *window);
        free(data);
    }
#endif

namespace houseofatmos {

    MainMenu::MainMenu(Settings&& settings):
            settings(std::move(settings)), 
            local_ref(this->settings.localization()) {
        u32 background_seed = random_init();
        this->terrain.generate_elevation(background_seed, 20.0, -5.0);
        this->terrain.generate_foliage(background_seed);
        this->renderer.camera.look_at = Vec<3>(12.5, 0.0, 32.0) 
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

    static void remove_missing_last_games(Settings& settings) {
        auto file_missing = [](const auto& path) { 
            return !std::filesystem::exists(path); 
        };
        std::erase_if(settings.last_games, file_missing);
        settings.save_to(Settings::default_path);
    }

    void MainMenu::load_game_from(
        const std::string& path, 
        const engine::Localization& local, engine::Window& window  
    ) {
        std::vector<char> data = engine::GenericLoader::read_bytes(path);
        this->load_game(data, path, local, window);
    }

    void MainMenu::load_game(
        std::span<const char> data, const std::string& path,
        const engine::Localization& local, engine::Window& window
    ) {
        auto buffer = engine::Arena(data);
        u32 format_version = buffer.get(engine::Arena::Position<u32>(0));
        u32 required_version = world::World::current_format_version;
        if(format_version != required_version) {
            std::string local_message = format_version < required_version
                ? "menu_save_file_too_old" : "menu_save_file_too_new";
            this->show_message(local_message, local, window);
            return;
        }
        auto world = std::make_shared<world::World>(
            Settings(this->settings), buffer
        );
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
            .with_pos(
                ui::horiz::in_window_fract(0.5), 
                ui::vert::in_window_fract(0.1)
            )
            .with_size(
                ui::unit * MainMenu::title_sprite.edge_size.x(),
                ui::unit * MainMenu::title_sprite.edge_size.y()
            )
            .with_background(&MainMenu::title_sprite)
            .as_movable()
        );
        ui::Element buttons = ui::Element()
            .with_pos(
                ui::horiz::in_window_fract(0.5), 
                ui::vert::in_window_fract(0.8)
            )
            .with_list_dir(ui::Direction::Vertical)
            .as_movable();
        buttons.children.push_back(ui_util::create_wide_button(
            local.text("menu_new_game"),
            [this, local = &local, window = &window]() {
                this->show_gamemode_screen(*local, *window);
            }
        ));
        #ifndef __EMSCRIPTEN__
            for(const std::string& game_path: this->settings.last_games) {
                std::string displayed_name 
                    = world::World::shortened_path(game_path);
                buttons.children.push_back(ui_util::create_wide_button(
                    local.pattern("menu_load_previous_game", { displayed_name }),
                    [window = &window, local = &local, this, path = game_path]() {
                        this->before_next_frame = [this, path, local, window]() {
                            this->load_game_from(path, *local, *window);
                        };
                        this->show_loading_screen(*local);
                    }
                ));
            }
            buttons.children.push_back(ui_util::create_wide_button(
                local.text("menu_load_game"),
                [local = &local, window = &window, this]() {
                    std::vector<std::string> chosen = pfd::open_file(
                        local->text("menu_choose_load_location"),
                        "",
                        { local->text("menu_save_file"), "*.bin" }
                    ).result();
                    if(chosen.empty()) { return; }
                    if(!std::filesystem::exists(chosen[0])) { return; }
                    this->before_next_frame 
                        = [this, path = chosen[0], local, window]() {
                            this->load_game_from(path, *local, *window);
                        };
                    this->show_loading_screen(*local);
                }
            ));
        #else
            buttons.children.push_back(ui_util::create_wide_button(
                local.text("menu_load_game"),
                [local = &local, window = &window, this]() {
                    hoa_read_file(this, window, local);
                }
            ));
        #endif
        buttons.children.push_back(ui_util::create_wide_button(
            local.text("menu_settings"),
            [this, local = &local, window = &window]() {
                this->show_settings(*local, *window);
            }
        ));
        #ifndef __EMSCRIPTEN__
            buttons.children.push_back(ui_util::create_wide_button(
                local.text("menu_exit_game"),
                []() { std::exit(0); }
            ));
        #endif
        this->ui.with_element(buttons
            .with_padding(5)
            .with_background(&ui_background::scroll_vertical)
            .as_movable()
        );
        this->ui.with_element(ui_util::create_icon(&ui_icon::earth)
            .with_pos(
                ui::width::window - ui::unit * 10 - ui::horiz::width, 
                ui::height::window - ui::unit * 10 - ui::vert::height
            )
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
        ui::Element notice = ui_util::create_text(
            local.text("menu_copyright_notice"), 0.0, &ui_font::bright
        );
        this->ui.with_element(notice
            .with_pos(
                ui::unit * 7.5, 
                ui::height::window - ui::unit * 7.5 - ui::vert::height
            )
            .as_movable()
        );
    }

    struct GameMode {
        std::string_view local_name;
        std::string_view local_descr;
        std::shared_ptr<engine::Scene> (*init)(
            std::shared_ptr<world::World> world
        );
    };

    static const std::vector<GameMode> game_modes = {
        GameMode(
            "menu_mode_tutorial", "menu_mode_descr_tutorial", 
            [](auto w) -> std::shared_ptr<engine::Scene> {
                return tutorial::create_toddler_scene(std::move(w));
            }
        ),
        GameMode(
            "menu_mode_classic", "menu_mode_descr_classic", 
            [](auto w) -> std::shared_ptr<engine::Scene> {
                return std::make_shared<world::Scene>(w);
            }
        ),
        GameMode(
            "menu_mode_creative", "menu_mode_descr_creative", 
            [](auto w) -> std::shared_ptr<engine::Scene> {
                w->balance.set_coins_silent(Balance::infinite_coins);
                size_t cond_c = research::Research::conditions().size();
                for(size_t cond_i = 0; cond_i < cond_c; cond_i += 1) {
                    const research::Research::ConditionInfo& cond_info
                        = research::Research::conditions().at(cond_i);
                    w->research.progress[(research::Research::Condition) cond_i]
                        .produced = cond_info.required;
                }
                return std::make_shared<world::Scene>(w);
            }
        )
    };

    void MainMenu::show_gamemode_screen(
        const engine::Localization& local, engine::Window& window
    ) {
        this->ui.root.children.clear();
        ui::Element modes = ui::Element()
            .with_pos(
                ui::horiz::in_window_fract(0.5), ui::vert::in_window_fract(0.5)
            )
            .with_list_dir(ui::Direction::Vertical)
            .as_movable();
        for(const GameMode& mode_info: game_modes) {
            ui::Element mode = ui::Element()
                .with_list_dir(ui::Direction::Vertical)
                .as_movable();
            mode.children.push_back(ui_util::create_button(
                local.text(mode_info.local_name),
                [window = &window, local = &local, i = mode_info.init, this]() {
                    this->before_next_frame = [window, init = i, this]() {
                        auto world_after = std::make_shared<world::World>(
                            Settings(this->settings), 256, 256
                        );
                        world_after->generate_map(random_init());
                        window->set_scene(init(world_after));
                    };
                    this->show_loading_screen(*local);
                }
            ));
            mode.children.push_back(
                ui_util::create_text(local.text(mode_info.local_descr), 0.0)
                    .with_size(ui::unit * 150, ui::height::text)
                    .with_padding(2.0)
                    .as_movable()
            );
            modes.children.push_back(mode.with_padding(4).as_movable());
        }
        ui::Element back = ui_util::create_button(
            local.text("ui_back"), 
            [this, local = &local, window = &window]() {
                this->show_title_screen(*local, *window);
            }
        );
        modes.children.push_back(std::move(back));
        this->ui.with_element(modes
            .with_padding(5)
            .with_background(&ui_background::scroll_vertical)
            .as_movable()
        );
    }

    void MainMenu::show_loading_screen(const engine::Localization& local) {
        this->ui.root.children.clear();
        ui::Element text = ui_util::create_text(
            local.text("menu_loading"), 0.0, &ui_font::bright
        );
        this->ui.with_element(text
            .with_pos(
                ui::horiz::in_window_fract(0.5), ui::vert::in_window_fract(0.5)
            )
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
            .with_pos(
                ui::horiz::in_window_fract(0.5), ui::vert::in_window_fract(0.5)
            )
            .with_list_dir(ui::Direction::Vertical)
            .as_movable();
        for(const auto& [name, locale]: languages) {
            selection.children.push_back(ui_util::create_button(
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
            local, window,
            [this, local = &local, window = &window]() {
                this->settings.save_to(Settings::default_path);
                this->show_title_screen(*local, *window);
            }
        ));
    }

    void MainMenu::show_message(
        std::string_view local_text,
        const engine::Localization& local, engine::Window& window
    ) {
        this->ui.root.children.clear();
        ui::Element container = ui::Element()
            .with_pos(
                ui::horiz::in_window_fract(0.5), ui::vert::in_window_fract(0.5)
            )
            .with_list_dir(ui::Direction::Vertical)
            .as_movable();
        container.with_child(
            ui_util::create_text(local.text(local_text) + "\n", 0)
                .with_size(ui::unit * 150, ui::height::text)
                .with_padding(2.0)
                .as_movable()
        );
        ui::Element back = ui_util::create_button(
            local.text("menu_close_menu"), 
            [this, local = &local, window = &window]() {
                this->show_title_screen(*local, *window);
            }
        );
        container.with_child(std::move(back));
        this->ui.with_element(container
            .with_padding(4.0)
            .with_background(&ui_background::scroll_horizontal)
            .as_movable()
        );
    }

    static const f64 camera_rotation_period = 60.0;
    static const f64 camera_y = 1.0; // 45 degrees
    static const f64 camera_distance = 40.0;

    void MainMenu::update(engine::Window& window) {
        this->settings.apply(*this, window);
        this->ui.unit_fract_size = this->settings.ui_size_fract();
        this->get(audio_const::soundtrack).update();
        this->get(audio_const::ambience).stop();
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
        world::Scene::configure_renderer(this->renderer, this->settings, 1.0);
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