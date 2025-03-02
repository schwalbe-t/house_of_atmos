
#pragma once

#include "toasts.hpp"

namespace houseofatmos {

    struct Balance {

        u64 coins;

        bool pay_coins(u64 amount, Toasts& toasts) {
            if(amount > this->coins) {
                toasts.add_error(
                    "toast_too_expensive", { std::to_string(amount) }
                );
                return false; 
            }
            this->coins -= amount;
            toasts.add_toast(
                "toast_removed_coins", { std::to_string(amount) }
            );
            return true;
        }

        void add_coins(u64 amount, Toasts& toasts) {
            this->coins += amount;
            toasts.add_toast(
                "toast_added_coins", { std::to_string(amount) }
            );
        }

        ui::Element create_counter(ui::Element** handle_ptr) {
            ui::Element counter = ui::Element()
                .with_handle(handle_ptr)
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(
                    std::to_string(this->coins) + " 🪙",
                    &ui_font::dark
                )
                .with_padding(1.0)
                .with_pos(15, 15, ui::position::window_tr_units)
                .with_background(&ui_background::note)
                .as_movable();
            return counter;
        }

        void update_counter(ui::Element& counter) {
            counter.text = std::to_string(this->coins) + " 🪙";
        }
    };

}