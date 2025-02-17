
#include "view.hpp"
#include "research.hpp"

namespace houseofatmos::research {

    View::View(
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
        this->load(engine::Shader::Loader(View::blur_shader));
    }


    
    static ui::Element create_icon_text(
        const ui::Background* icon, std::string text, const ui::Font* font
    ) {
        ui::Element container = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Horizontal)
            .as_movable();
        container.children.push_back(ui::Element()
            .with_size(icon->edge_size.x(), icon->edge_size.y(), ui::size::units)
            .with_background(icon)
            .as_movable()
        );
        container.children.push_back(ui::Element()
            .with_size(
                4.0, (icon->edge_size.y() - font->height) / 2.0, 
                ui::size::units_with_children
            )
            .with_child(ui::Element()
                .with_pos(1.0, 0.5, ui::position::parent_offset_fract)
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(text, font)
                .as_movable()
            )
            .as_movable()
        );
        return container;
    }

    static ui::Element create_research_item(
        const engine::Localization& local, 
        const Research& research, Research::Advancement advancement
    ) {
        const Research::AdvancementInfo& advancement_info 
            = Research::advancements.at(advancement);
        const Research::AdvancementProgress& progress 
            = research.progress.at(advancement);
        bool parents_unlocked = research.parents_unlocked(advancement);
        const ui::Background* background = parents_unlocked
            ? &ui_background::note : &ui_background::border_dark;
        const ui::Font* font = parents_unlocked
            ? &ui_font::dark : &ui_font::bright;
        ui::Element content = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Vertical)
            .as_movable();
        std::string title = local.text(advancement_info.local_name);
        if(progress.is_unlocked) { title += " ✅"; }
        content.children.push_back(
            create_icon_text(advancement_info.icon, title, font)
                .with_padding(2.0)
                .as_movable()
        );
        ui::Element requirements = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Vertical)
            .as_movable();
        requirements.children.push_back(ui::Element()
            .with_size(0, 0, ui::size::unwrapped_text)
            .with_text(local.text("ui_required"), font)
            .with_padding(3.0)
            .as_movable()
        );
        for(Research::Advancement parent: advancement_info.parents) {
            const Research::AdvancementInfo& parent_info 
                = Research::advancements.at(parent);
            std::string text = local.text(parent_info.local_name);
            if(research.is_unlocked(parent)) { text += " ✅"; }
            requirements.children.push_back(
                create_icon_text(parent_info.icon, text, font)
                    .with_padding(1.0)
                    .as_movable()
            );
        }
        size_t item_c = progress.item_conditions.size();
        for(size_t item_i = 0; item_i < item_c; item_i += 1) {
            u64 produced = progress.item_conditions[item_i].produced_count;
            u64 required = advancement_info.item_conditions[item_i].required_count;
            const world::Item::TypeInfo& item_info = world::Item::items
                .at((size_t) advancement_info.item_conditions[item_i].item);
            std::string text = std::to_string(produced);
            text += "/" + std::to_string(required);
            text += " " + local.text(item_info.local_name);
            if(produced >= required) { text += " ✅"; }
            requirements.children.push_back(
                create_icon_text(item_info.icon, text, font)
                    .with_padding(1.0)
                    .as_movable()
            );
        }
        content.children.push_back(requirements
            .with_padding(2.0)
            .as_movable()
        );
        ui::Element container = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_pos(0.5, 0.5, ui::position::parent_offset_fract)
            .with_background(background)
            .with_child(content
                .with_padding(2.0)
                .as_movable()
            )
            .as_movable();
        Vec<2> pos = advancement_info.view_pos;
        ui::Element wrapper = ui::Element()
            .with_pos(pos.x(), pos.y(), ui::position::parent_offset_fract)
            .with_size(0, 0, ui::size::units)
            .with_child(std::move(container))
            .as_movable();
        return wrapper;
    }

    void View::init_ui() {
        Toasts::States toast_states = this->toasts.make_states();
        this->ui.root.children.clear();
        const engine::Localization& local = this
            ->get<engine::Localization>(this->world->settings.localization());
        ui::Element view = ui::Element()
            .with_pos(0.5, 0.5, ui::position::parent_offset_fract)
            .with_size(1.0, 1.0, ui::size::window_fract)
            .as_movable();
        for(const auto& [advancement, info]: Research::advancements) {
            view.children.push_back(create_research_item(
                local, 
                this->world->research, advancement
            ));
        }
        this->ui.with_element(ui::Element()
            .with_handle(&this->view_root)
            .with_pos(
                this->view_offset.x() * 2, -this->view_offset.y() * 2, 
                ui::position::window_ndc
            )
            .with_size(0, 0, ui::size::units)
            .with_child(std::move(view))
            .as_movable()
        );
        // add elements here!
        this->toasts.set_scene(this);
        this->ui.with_element(this->toasts.create_container());
        this->toasts.put_states(std::move(toast_states));
    }

    static const f64 view_update_time = 1.0 / 10.0;

    void View::update_view(const engine::Window& window) {
        bool mouse_down = window.is_down(engine::Button::Left)
            || window.is_down(engine::Button::Middle)
            || window.is_down(engine::Button::Right);
        Vec<2> cursor_pos = window.cursor_pos_px() 
            / Vec<2>(window.width(), window.height());
        bool started_dragging = mouse_down && !this->view_anchor.has_value();
        if(started_dragging) {
            this->view_anchor = (Anchor) { cursor_pos, this->view_offset };
        }
        if(this->view_anchor.has_value()) {
            Vec<2> distance = cursor_pos - this->view_anchor->cursor_pos;
            this->view_offset = this->view_anchor->view_offset + distance;
        }
        bool stopped_dragging = !mouse_down && this->view_anchor.has_value();
        if(stopped_dragging) {
            this->view_anchor = std::nullopt;
        }
        this->view_root->position.x() = this->view_offset.x() * 2;
        this->view_root->position.y() = -this->view_offset.y() * 2;
        this->view_update_timer += window.delta_time();
        if(this->view_update_timer >= view_update_time) {
            this->view_update_timer = 0.0;
            this->init_ui();
        }
    }



    void View::update(engine::Window& window) {
        this->world->settings.apply(*this, window);
        this->get<engine::Soundtrack>(audio_const::soundtrack).update();
        if(this->ui.root.children.size() == 0) {
            this->init_ui();
        }
        bool was_closed = window.was_pressed(engine::Key::E)
            || window.was_pressed(engine::Key::Escape);
        if(was_closed) {
            window.set_scene(std::shared_ptr<engine::Scene>(this->previous));
        }
        this->world->carriages.update_all(
            Vec<3>(0.0, 0.0, 0.0), 0.0,
            *this, window, 
            this->world->complexes, this->world->terrain, this->toasts
        );
        this->world->complexes.update(
            window, this->world->balance, this->world->research
        );
        this->world->research.check_completion(this->toasts);
        this->update_view(window);
        this->toasts.update(*this);
        this->ui.update(window);
    }

    void View::render(engine::Window& window) {
        if(!this->background.has_value()) {
            this->background = engine::Texture(
                this->last_frame.width(), this->last_frame.height()
            );
            engine::Shader& blur = this->get<engine::Shader>(View::blur_shader);
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