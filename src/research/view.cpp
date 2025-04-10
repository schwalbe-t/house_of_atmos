
#include "view.hpp"
#include "research.hpp"
#include "../ui_util.hpp"
#include <algorithm>

namespace houseofatmos::research {

    static ui::Background tree_background = ui::Background(
        View::background_design, Vec<2>(0, 0),
        Vec<2>(0, 0), Vec<2>(200, 128)
    );

    struct ConditionDisplay {
        ui::Background unlock_overlay;
        Vec<2> overlay_offset;
        const ui::Background* icon;
        Vec<2> icon_offset;
    };

    static std::vector<ConditionDisplay> condition_displays = {
        /* SteamEngines */ {
            ui::Background(
                View::background_design, Vec<2>(200, 0),
                Vec<2>(0, 0), Vec<2>(200, 104)
            ),
            Vec<2>(0, 0),
            &ui_icon::steam_engines,
            Vec<2>(12, 12)
        },
        /* BrassPots */ {
            ui::Background(
                View::background_design, Vec<2>(200, 104),
                Vec<2>(0, 0), Vec<2>(104, 32)
            ),
            Vec<2>(24, 24),
            &ui_icon::brass_pots,
            Vec<2>(36, 36)
        },
        /* PowerLooms */ {
            ui::Background(
                View::background_design, Vec<2>(200, 136),
                Vec<2>(0, 0), Vec<2>(80, 32)
            ),
            Vec<2>(24, 48),
            &ui_icon::power_looms,
            Vec<2>(36, 60)
        },
        /* SteelBeams */ {
            ui::Background(
                View::background_design, Vec<2>(200, 168),
                Vec<2>(0, 0), Vec<2>(104, 56)
            ),
            Vec<2>(24, 72),
            &ui_icon::steel_beams,
            Vec<2>(36, 84)
        },
        /* CoalLocomotives */ {
            ui::Background(
                View::background_design, Vec<2>(200, 224),
                Vec<2>(0, 0), Vec<2>(104, 32)
            ),
            Vec<2>(48, 96),
            &ui_icon::locomotive_frames,
            Vec<2>(60, 108)
        }
    };

    struct RewardDisplay {
        const ui::Background* icon;
        Vec<2> icon_offset;
    };

    static std::vector<RewardDisplay> reward_displays = {
        /* Steel */ { &ui_icon::steel, Vec<2>(36, 12) },
        /* SteelBeams */ { &ui_icon::steel_beams, Vec<2>(60, 12) },
        /* BrassPots */ { &ui_icon::brass_pots, Vec<2>(84, 12) },
        /* Oil */ { &ui_icon::oil, Vec<2>(108, 12) },
        /* OilLanterns */ { &ui_icon::oil_lanterns, Vec<2>(132, 12) },
        /* Watches */ { &ui_icon::watches, Vec<2>(156, 12) },
        /* PowerLooms */ { &ui_icon::power_looms, Vec<2>(180, 12) },

        /* Steak */ { &ui_icon::steak, Vec<2>(60, 36) },
        /* Cheese */ { &ui_icon::cheese, Vec<2>(84, 36) },
        /* Beer */ { &ui_icon::beer, Vec<2>(108, 36) },

        /* Fabric */ { &ui_icon::fabric, Vec<2>(60, 60) },
        /* Clothing */ { &ui_icon::clothing, Vec<2>(84, 60) },

        /* Tracking */ { &ui_icon::tracking, Vec<2>(56, 80) },
        /* LocomotiveFrames */ { &ui_icon::locomotive_frames, Vec<2>(84, 84) },
        /* SteelBridges */ { &ui_icon::metal_bridge, Vec<2>(104, 80) },

        /* BasicLocomotive */ { &ui_icon::basic_locomotive, Vec<2>(80, 104) },
        /* SmallLocomotive */ { &ui_icon::small_locomotive, Vec<2>(104, 104) },
        /* Tram */ { &ui_icon::tram, Vec<2>(128, 104) }
    };

    ui::Element View::build_research_tree() {
        Vec<2> root_size = tree_background.edge_size;
        ui::Element root = ui::Element()
            .with_size(root_size.x(), root_size.y(), ui::size::units)
            .with_background(&tree_background)
            .as_movable();
        size_t cond_c = Research::conditions().size();
        for(size_t c = cond_c - 1; cond_c >= 1; c -= 1) {
            bool is_unlocked = this->world->research
                .is_unlocked((Research::Condition) c);
            bool is_selected = this->selected_cond.has_value()
                && (size_t) (*this->selected_cond) == c;
            const ConditionDisplay& display = condition_displays[c];
            Vec<2> overlay_size = display.unlock_overlay.edge_size;
            ui::Element overlay = ui::Element()
                .with_size(overlay_size.x(), overlay_size.y(), ui::size::units)
                .with_pos(
                    display.overlay_offset.x(), display.overlay_offset.y(), 
                    ui::position::parent_offset_units
                )
                .with_background(is_unlocked? &display.unlock_overlay : nullptr)
                .with_click_handler([this, c, is_selected]() {
                    if(is_selected) { return; }
                    this->selected_cond = (Research::Condition) c;
                    this->view_update_timer = INFINITY;
                })
                .as_movable();
            root.children.push_back(std::move(overlay));
            if(c == 0) { break; }
        }
        for(size_t c = 0; c < Research::conditions().size(); c += 1) {
            const ConditionDisplay& display = condition_displays[c];
            bool is_selected = this->selected_cond.has_value()
                && (size_t) (*this->selected_cond) == c;
            root.children.push_back(ui_util::create_icon(display.icon)
                .with_pos(
                    display.icon_offset.x(), display.icon_offset.y(), 
                    ui::position::parent_offset_units
                )
                .with_padding(0.0)
                .as_phantom()
                .with_background(
                    is_selected? &ui_background::border_selected : nullptr
                )
                .as_movable()
            );
        }
        for(size_t r = 0; r < Research::rewards().size(); r += 1) {
            const RewardDisplay& display = reward_displays[r];
            root.children.push_back(ui_util::create_icon(display.icon)
                .with_pos(
                    display.icon_offset.x(), display.icon_offset.y(), 
                    ui::position::parent_offset_units
                )
                .as_movable()
            );
        }
        return root;
    }

    ui::Element View::build_condition_display(
        Research::Condition cond, const engine::Localization& local
    ) {
        ui::Element root = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Vertical)
            .as_movable();
        const Research::ConditionInfo& cond_info = Research::conditions()
            .at((size_t) cond);
        const ConditionDisplay& cond_display = condition_displays
            .at((size_t) cond);
        bool is_unlocked = this->world->research.is_unlocked(cond);
        root.children.push_back(ui_util::create_icon_with_text(
            cond_display.icon, 
            local.text(cond_info.local_name) + (is_unlocked? "✅" : ""), 
            4.0
        ));
        if(cond_info.parents.size() > 0) {
            root.children.push_back(ui_util::create_text(
                local.text("ui_condition_required_conditions")
            ));
            for(const Research::Condition& parent: cond_info.parents) {
                const Research::ConditionInfo& parent_info 
                    = Research::conditions().at((size_t) parent);
                const ConditionDisplay& parent_display
                    = condition_displays.at((size_t) parent);
                bool par_unlocked = this->world->research.is_unlocked(parent);
                root.children.push_back(ui_util::create_icon_with_text(
                    parent_display.icon, 
                    local.text(parent_info.local_name) 
                        + (par_unlocked? "✅" : ""),
                    1.0
                ));
            }
        }
        root.children.push_back(
            ui_util::create_text(local.text("ui_condition_required_items"))
        );
        const world::Item::TypeInfo& item_info = world::Item::types()
            .at((size_t) cond_info.item);
        const Research::ConditionInfo::Progress& progress = this->world
            ->research.progress.at(cond);
        root.children.push_back(ui_util::create_icon_with_text(
            item_info.icon, 
            std::to_string(progress.produced) + "/" 
                + std::to_string(cond_info.required) + " " 
                + local.text(item_info.local_name)
                + (progress.produced >= cond_info.required? "✅" : ""),
            1.0
        ));
        root.children.push_back(
            ui_util::create_text(local.text("ui_condition_rewards"))
        );
        for(size_t r = 0; r < Research::rewards().size(); r += 1) {
            const Research::RewardInfo& reward_info = Research::rewards()
                .at((size_t) r);
            const auto& req = reward_info.required;
            bool is_reward = std::find(req.begin(), req.end(), cond) 
                != req.end();
            if(!is_reward) { continue; }
            const RewardDisplay& reward_display = reward_displays
                .at((size_t) r);
            root.children.push_back(ui_util::create_icon_with_text(
                reward_display.icon, 
                local.text(reward_info.local_name) + (is_unlocked? "✅" : ""),
                1.0
            ));
        }
        ui::Element padded = root
            .with_padding(3.0)
            .with_background(&ui_background::scroll_vertical)
            .as_movable();
        return padded;
    }
    


    View::View(
        std::shared_ptr<world::World>&& world,
        std::shared_ptr<Scene>&& previous
    ): toasts(Toasts(world->settings)) {
        this->world = std::move(world);
        this->previous = std::move(previous);
        ui::Manager::load_shaders(*this);
        ui_const::load_all(*this);
        audio_const::load_all(*this);
        this->load(this->world->settings.localization());
        this->load(View::background_design);
    }

    void View::init_ui(engine::Window& window) {
        Toasts::States toast_states = this->toasts.make_states();
        this->ui.root.children.clear();
        const engine::Localization& local
            = this->get(this->world->settings.localization());
        this->ui.with_element(
            this->build_research_tree()
                .with_pos(0.85, 0.5, ui::position::window_fract)
                .as_movable()
        );
        if(this->selected_cond.has_value()) {
            this->ui.with_element(
                this->build_condition_display(*this->selected_cond, local)
                    .with_pos(0.15, 0.5, ui::position::window_fract)
                    .as_movable()
            );
        }
        ui::Element back_button = ui_util::create_button(
            local.text("ui_back"),
            [this, window = &window]() {
                window->set_scene(
                    std::shared_ptr<engine::Scene>(this->previous)
                );
            }
        );
        this->ui.with_element(back_button
            .with_pos(10, 10, ui::position::window_bl_units)
            .as_movable()
        );
        this->toasts.set_scene(this);
        this->ui.with_element(this->toasts.create_container());
        this->toasts.put_states(std::move(toast_states));
    }

    static const f64 view_update_time = 1.0;

    void View::update(engine::Window& window) {
        this->world->settings.apply(*this, window);
        this->get(audio_const::soundtrack).update();
        bool was_closed = window.was_pressed(engine::Key::E)
            || window.was_pressed(engine::Key::Escape);
        if(was_closed) {
            window.set_scene(std::shared_ptr<engine::Scene>(this->previous));
        }
        this->world->update(*this, window, this->toasts);
        this->view_update_timer += window.delta_time();
        if(this->view_update_timer >= view_update_time) {
            this->view_update_timer = 0.0;
            this->init_ui(window);
        }
        this->toasts.update(*this);
        this->ui.update(window);
    }

    static const Vec<3> background_color = Vec<3>(31, 14, 28) / 255.0;

    void View::render(engine::Window& window) {
        this->background.resize_fast(window.width(), window.height());
        this->background.as_target().clear_color(background_color.with(1.0));
        window.show_texture(this->background);
        this->ui.render(*this, window);
        window.show_texture(this->ui.output());
    }

}