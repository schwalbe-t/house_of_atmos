
#include <samhocevar/portable-file-dialogs.h>
#include "pause_menu.hpp"
#include "../ui_util.hpp"
#include "../main_menu/main_menu.hpp"
#include <algorithm>

namespace houseofatmos {

    PauseMenu::PauseMenu(
        std::shared_ptr<world::World>&& world,
        std::shared_ptr<Scene>&& previous, 
        const engine::Texture& last_frame
    ): last_frame(last_frame), toasts(Toasts(world->settings)) {
        this->world = std::move(world);
        this->previous = std::move(previous);
        ui::Manager::load_shaders(*this);
        ui_const::load_all(*this);
        audio_const::load_all(*this);
        this->load(this->world->settings.localization());
        this->load(PauseMenu::blur_shader);
    }

    void PauseMenu::save_game(engine::Window& window, bool is_new_save) {
        if(!this->world->write_to_file(is_new_save)) {
            this->world->save_path = "";
            this->toasts.add_error("toast_failed_to_save_game", {});
            this->show_root_menu(window);
            return;
        }
        this->world->settings.add_recent_game(
            std::string(this->world->save_path)
        );
        this->world->settings.save_to(Settings::default_path);
        this->toasts.add_toast("toast_saved_game", {
            world::World::shortened_path(this->world->save_path) 
        });
    }

    void PauseMenu::show_root_menu(engine::Window& window) {
        Toasts::States toast_states = this->toasts.make_states();
        this->ui.root.children.clear();
        const engine::Localization& local 
            = this->get(this->world->settings.localization());
        this->ui.with_element(ui::Element()
            .with_pos(
                ui::horiz::in_window_fract(0.5),
                ui::horiz::in_window_fract(0.2)
            )
            .with_size(ui::width::text, ui::height::text)
            .with_text(local.text("menu_game_paused"), &ui_font::bright)
            .as_movable()
        );
        ui::Element buttons = ui::Element()
            .with_list_dir(ui::Direction::Vertical)
            .as_movable();
        buttons.children.push_back(ui_util::create_wide_button(
            local.text("menu_resume_game"), 
            [window = &window, this]() {
                window->set_scene(std::shared_ptr<engine::Scene>(this->previous)); 
            }
        ));
        #ifdef __EMSCRIPTEN__
            if(this->world->saving_allowed) {
                buttons.children.push_back(ui_util::create_wide_button(
                    local.text("menu_save_game"),
                    [this, window = &window]() {
                        this->save_game(*window); 
                    }
                ));
            }
        #else
            if(this->world->saving_allowed && this->world->save_path.size() > 0) {
                buttons.children.push_back(ui_util::create_wide_button(
                    local.text("menu_save_game"),
                    [this, window = &window]() {
                        this->save_game(*window); 
                    }
                ));
            }
            if(this->world->saving_allowed) {
                buttons.children.push_back(ui_util::create_wide_button(
                    local.text("menu_save_game_as"),
                    [this, local = &local, window = &window]() {
                        std::string new_path = pfd::save_file(
                            local->text("menu_choose_save_location"),
                            "",
                            { local->text("menu_save_file"), "*.bin" }
                        ).result();
                        if(new_path.size() == 0) {
                            this->toasts.add_error("toast_failed_to_save_game", {});
                            return;
                        }
                        this->world->save_path = std::move(new_path); 
                        this->save_game(*window, true /* = new save location */);
                        this->show_root_menu(*window);
                    }
                ));
            }
        #endif
        buttons.children.push_back(ui_util::create_wide_button(
            local.text("menu_settings"),
            [window = &window, this]() {
                this->show_settings(*window);
            }
        ));
        buttons.children.push_back(ui_util::create_wide_button(
            local.text("menu_to_main_menu"),
            [window = &window, this]() {
                #ifndef __EMSCRIPTEN__
                    this->world->write_to_file();
                #endif
                window->set_scene(std::make_shared<MainMenu>(
                    Settings(this->world->settings)
                ));
            }
        ));
        this->ui.with_element(ui::Element()
            .with_pos(
                ui::horiz::in_window_fract(0.5), ui::vert::in_window_fract(0.7)
            )
            .with_child(std::move(buttons))
            .with_padding(5)
            .with_background(&ui_background::scroll_vertical)
            .as_movable()
        );
        this->toasts.set_scene(this);
        this->ui.with_element(this->toasts.create_container());
        this->toasts.put_states(std::move(toast_states));
    }
    
    void PauseMenu::show_settings(engine::Window& window) {
        Toasts::States toast_states = this->toasts.make_states();
        this->ui.root.children.clear();
        const engine::Localization& local
            = this->get(this->world->settings.localization());
        this->ui.with_element(this->world->settings.create_menu(
            local, window,
            [this, window = &window]() {
                this->world->settings.save_to(Settings::default_path);
                this->show_root_menu(*window);
            }
        ));
        this->toasts.set_scene(this);
        this->ui.with_element(this->toasts.create_container());
        this->toasts.put_states(std::move(toast_states));
    }

    void PauseMenu::update(engine::Window& window) {
        this->world->settings.apply(*this, window);
        this->ui.unit_fract_size = this->world->settings.ui_size_fract();
        this->get(audio_const::soundtrack).update();
        this->get(audio_const::ambience).stop();
        if(this->ui.root.children.size() == 0) {
            this->show_root_menu(window);
        }
        if(window.was_pressed(engine::Key::Escape)) {
            this->world->settings.save_to(Settings::default_path);
            window.set_scene(std::shared_ptr<engine::Scene>(this->previous));
        }
        this->toasts.update(*this);
        this->ui.update(window);
    }

    void PauseMenu::render(engine::Window& window) {
        if(!this->background.has_value()) {
            this->background = engine::Texture(
                this->last_frame.width(), this->last_frame.height()
            );
            engine::Shader& blur = this
                ->get(PauseMenu::blur_shader);
            blur.set_uniform("u_texture_w", (i64) this->background->width());
            blur.set_uniform("u_texture_h", (i64) this->background->height());
            i64 blur_rad = (i64) ((window.width() + window.height()) / 200.0);
            blur.set_uniform("u_blur_rad", blur_rad);
            this->last_frame.blit(this->background->as_target(), blur);
        }
        window.show_texture(*this->background);
        this->ui.render(*this, window);
        window.show_texture(this->ui.output());
    }

}