
#pragma once

#include <engine/ui.hpp>
#include <engine/localization.hpp>
#include "ui_background.hpp"
#include "ui_font.hpp"

namespace houseofatmos {

    namespace ui = houseofatmos::engine::ui;


    struct Toasts {

        struct States {
            std::vector<ui::Element> elements;
            std::vector<f64> timers;
        };


        static inline std::vector<ui::Element> empty_toasts;
        static inline const size_t max_toast_count = 10;

        private:
        const engine::Localization::LoadArgs& local_ref;
        const engine::Localization* local = nullptr;
        ui::Element* toasts_container = nullptr;
        std::vector<f64> toast_timers;

        public:
        Toasts(const engine::Localization::LoadArgs& l): local_ref(l) {}

        ui::Element create_container() {
            ui::Element container = ui::Element()
                .with_handle(&this->toasts_container)
                .with_pos(10, 10, ui::position::window_br_units)
                .with_size(0, 0, ui::size::units_with_children)
                .as_movable();
            return container;
        }

        std::vector<ui::Element>& elements() {
            if(this->toasts_container == nullptr) {
                empty_toasts.clear();
                return empty_toasts;
            }
            return toasts_container->children;
        }

        void add_toast(
            std::string name, std::span<const std::string> values, 
            f64 time = 5.0
        ) {
            if(this->local == nullptr) { return; }
            if(this->toasts_container == nullptr) {
                this->toast_timers.clear();
            }
            if(this->toast_timers.size() == Toasts::max_toast_count) {
                this->toast_timers.erase(this->toast_timers.begin());
                this->elements().erase(this->elements().begin());
            }
            toast_timers.push_back(time);
            this->elements().push_back(ui::Element()
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(
                    this->local->pattern(name, values), 
                    &ui_font::standard
                )
                .with_background(&ui_background::note)
                .with_padding(5.0)
                .as_movable()
            );
        }

        void add_toast(
            std::string name, std::initializer_list<const std::string> values, 
            f64 time = 5.0
        ) {
            this->add_toast(name, std::span(values), time);
        }

        States make_states() {
            if(this->toasts_container == nullptr) {
                this->toast_timers.clear();
            }
            return (States) {
                std::move(this->elements()),
                std::move(this->toast_timers)
            };
        }

        void put_states(States&& states) {
            this->elements() = std::move(states.elements);
            this->toast_timers = std::move(states.timers);
        }

        void update(const engine::Window& window, engine::Scene& scene) {
            this->local = &scene.get<engine::Localization>(this->local_ref);
            if(this->toasts_container == nullptr) {
                this->toast_timers.clear();
            }
            for(size_t i = 0; i < this->toast_timers.size();) {
                f64& time = this->toast_timers[i];
                time -= window.delta_time();
                if(time >= 0.0) {
                    i += 1;
                    continue;
                }
                this->elements().erase(this->elements().begin() + i);
                this->toast_timers.erase(this->toast_timers.begin() + i);
            }
        }

    };

}