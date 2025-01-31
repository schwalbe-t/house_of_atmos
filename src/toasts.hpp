
#pragma once

#include <engine/ui.hpp>
#include <engine/localization.hpp>
#include "ui_const.hpp"
#include <chrono>

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

        static f64 current_time() {
            return std::chrono::duration<f64>(
                std::chrono::high_resolution_clock::now().time_since_epoch()
            ).count();
        }

        public:
        Toasts(const engine::Localization::LoadArgs& l): local_ref(l) {}

        const engine::Localization& localization() const { 
            return *this->local; 
        }

        ui::Element create_container() {
            ui::Element container = ui::Element()
                .as_phantom()
                .with_handle(&this->toasts_container)
                .with_pos(10, 10, ui::position::window_tl_units)
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
            f64 duration = 5.0, 
            const ui::Background* background = &ui_background::note,
            const ui::Font* font = &ui_font::dark
        ) {
            if(this->local == nullptr) {
                return; 
            }
            if(this->toasts_container == nullptr) {
                this->toast_timers.clear();
                return;
            }
            if(this->toast_timers.size() == Toasts::max_toast_count) {
                this->toast_timers.erase(this->toast_timers.begin());
                this->elements().erase(this->elements().begin());
            }
            f64 death_time = Toasts::current_time() + duration;
            toast_timers.push_back(death_time);
            this->elements().push_back(ui::Element()
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(this->local->pattern(name, values), font)
                .with_background(background)
                .with_padding(5.0)
                .as_movable()
            );
        }

        void add_toast(
            std::string name, std::initializer_list<const std::string> values, 
            f64 duration = 5.0
        ) {
            this->add_toast(name, std::span(values), duration);
        }

        void add_error(
            std::string name, std::initializer_list<const std::string> values, 
            f64 duration = 5.0
        ) {
            this->add_toast(
                name, std::span(values), duration, 
                &ui_background::note_error, &ui_font::bright
            );
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

        void update(engine::Scene& scene) {
            this->local = &scene.get<engine::Localization>(this->local_ref);
            if(this->toasts_container == nullptr) {
                this->toast_timers.clear();
            }
            f64 curr_time = Toasts::current_time();
            for(size_t i = 0; i < this->toast_timers.size();) {
                const f64& death_time = this->toast_timers[i];
                if(curr_time < death_time) {
                    i += 1;
                    continue;
                }
                this->elements().erase(this->elements().begin() + i);
                this->toast_timers.erase(this->toast_timers.begin() + i);
            }
        }

    };

}