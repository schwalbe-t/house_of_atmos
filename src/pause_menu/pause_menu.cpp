
#include <samhocevar/portable-file-dialogs.h>
#include "pause_menu.hpp"
#include "../main_menu/main_menu.hpp"
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace houseofatmos {

    PauseMenu::PauseMenu(
        std::shared_ptr<world::World>&& world,
        std::shared_ptr<Scene>&& previous, 
        const engine::Texture& last_frame
    ): last_frame(last_frame), toasts(Toasts(world->settings.localization())) {
        this->world = std::move(world);
        this->previous = std::move(previous);
        ui::Manager::load_shaders(*this);
        ui_const::load_all(*this);
        audio_const::load_all(*this);
        this->load(engine::Localization::Loader(
            this->world->settings.localization()
        ));
        this->load(engine::Shader::Loader(PauseMenu::blur_shader));
    }

    static void write_to_file(
        const engine::Arena& buffer, const std::string& path
    ) {
        std::ofstream fout;
        fout.open(path, std::ios::binary | std::ios::out);
        fout.write((const char*) buffer.data().data(), buffer.data().size());
        fout.close();
    }

    void PauseMenu::save_game(engine::Window& window, bool is_new_save) {
        bool existing_path_invalid = !is_new_save
            && !std::filesystem::exists(this->world->save_path);
        if(existing_path_invalid) {
            this->world->save_path = "";
            this->toasts.add_error("toast_failed_to_save_game", {});
            this->refresh_ui_elements(window);
            return;
        }
        engine::Arena serialized = this->world->serialize();
        write_to_file(serialized, this->world->save_path);
        this->toasts.add_toast("toast_saved_game", { this->world->save_path });
        this->world->settings.add_recent_game(
            std::string(this->world->save_path)
        );
        this->world->settings.save_to(Settings::default_path);
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

    void PauseMenu::refresh_ui_elements(engine::Window& window) {
        Toasts::States toast_states = this->toasts.make_states();
        this->ui.root.children.clear();
        const engine::Localization& local = this
            ->get<engine::Localization>(this->world->settings.localization());
        this->ui.with_element(ui::Element()
            .with_pos(0.5, 0.2, ui::position::window_fract)
            .with_size(0.0, 0.0, ui::size::unwrapped_text)
            .with_text(local.text("ui_game_paused"), &ui_font::bright)
            .as_movable()
        );
        ui::Element buttons = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Vertical)
            .as_movable();
        buttons.children.push_back(make_button(
            local.text("ui_resume_game"), 
            [window = &window, this]() {
                window->set_scene(std::shared_ptr<engine::Scene>(this->previous)); 
            }
        ));
        if(this->world->save_path.size() > 0) {
            buttons.children.push_back(make_button(
                local.text("ui_save_game"),
                [this, window = &window]() {
                    this->save_game(*window); 
                }
            ));
        }
        buttons.children.push_back(make_button(
            local.text("ui_save_game_as"),
            [this, local = &local, window = &window]() {
                std::string new_path = pfd::save_file(
                    local->text("ui_choose_save_location"),
                    "",
                    { local->text("ui_save_file"), "*.bin" }
                ).result();
                if(new_path.size() == 0) {
                    this->toasts.add_error("toast_failed_to_save_game", {});
                    return;
                }
                this->world->save_path = std::move(new_path); 
                this->save_game(*window, true /* = new save location */);
                this->refresh_ui_elements(*window);
            }
        ));
        buttons.children.push_back(make_button(
            local.text("ui_to_main_menu"),
            [window = &window, this]() {
                window->set_scene(std::make_shared<MainMenu>(
                    Settings(this->world->settings)
                ));
            }
        ));
        this->ui.with_element(ui::Element()
            .with_pos(0.5, 0.7, ui::position::window_fract)
            .with_size(0.0, 0.0, ui::size::units_with_children)
            .with_child(std::move(buttons))
            .with_padding(5)
            .with_background(&ui_background::scroll_vertical)
            .as_movable()
        );
        this->toasts.set_scene(this);
        this->ui.with_element(this->toasts.create_container());
        this->toasts.put_states(std::move(toast_states));
    }

    void PauseMenu::update(engine::Window& window) {
        this->get<engine::Soundtrack>(audio_const::soundtrack).update();
        if(this->ui.root.children.size() == 0) {
            this->refresh_ui_elements(window);
        }
        if(window.was_pressed(engine::Key::Escape)) {
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
                ->get<engine::Shader>(PauseMenu::blur_shader);
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