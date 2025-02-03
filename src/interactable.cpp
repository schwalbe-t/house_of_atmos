
#include "interactable.hpp"
#include "ui_const.hpp"
#include <algorithm>

namespace houseofatmos {

    ui::Element Interactables::create_container() {
        ui::Element container = ui::Element()
            .as_phantom()
            .with_handle(&this->container)
            .with_pos(0, 0, ui::position::window_fract)
            .with_size(1, 1, ui::size::window_fract)
            .as_movable();
        return container;
    }

    std::shared_ptr<Interactable> Interactables::create(
        std::function<void ()>&& handler, Vec<3> pos
    ) {
        auto instance = std::shared_ptr<Instance>(
            new Instance(std::move(handler), pos)
        );
        this->instances.push_back(instance);
        return instance;
    }

    void Interactables::observe_from(
        const Vec<3>& pos,
        const Renderer& renderer, const engine::Window& window
    ) {
        this->clean_instance_refs();
        this->update_ui_elements(pos, renderer, window);
    }

    void Interactables::clean_instance_refs() {
        auto is_expired = [](const auto& ref) { return ref.expired(); };
        std::erase_if(this->instances, is_expired);
    }

    static inline const f64 max_interaction_distance = 2.5;

    void Interactables::update_ui_elements(
        const Vec<3>& pos, 
        const Renderer& renderer, const engine::Window& window
    ) {
        if(this->container == nullptr) { return; }
        this->container->children.clear();
        std::shared_ptr<Interactable> closest_inst = nullptr;
        ui::Element* closest_elem = nullptr;
        f64 closest_dist = INFINITY;
        for(const auto& weak_instance: this->instances) {
            std::shared_ptr<Interactable> instance = weak_instance.lock();
            if(instance == nullptr) { continue; }
            Vec<2> pos_ndc = renderer.world_to_ndc(instance->pos);
            if(pos_ndc.abs().min() >= 1.1) { continue; }
            f64 dist = (instance->pos - pos).len();
            if(dist > max_interaction_distance) { continue; }
            this->container->children.push_back(ui::Element()
                .with_pos(pos_ndc.x(), pos_ndc.y(), ui::position::window_ndc)
                .with_size(7, 7, ui::size::units)
                .with_background(
                    &ui_background::border, &ui_background::border_hovering
                )
                .with_click_handler([w_inst = &weak_instance]() {
                    std::shared_ptr<Interactable> inst = w_inst->lock();
                    if(inst == nullptr) { return; }
                    inst->handler();
                })
                .as_movable()
            );
            if(dist >= closest_dist) { continue; }
            closest_inst = instance;
            closest_elem = &this->container->children.back();
            closest_dist = dist;
        }
        if(closest_elem != nullptr) {
            closest_elem->background = &ui_background::border_selected;
            closest_elem->children.push_back(ui::Element()
                .as_phantom()
                .with_pos(0.5, 0.5, ui::position::parent_offset_fract)
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text("E", &ui_font::bright)
                .as_movable()
            );
        }
        if(closest_inst != nullptr && window.was_pressed(engine::Key::E)) {
            closest_inst->handler();
        }
    }

}