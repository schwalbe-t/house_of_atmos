
#pragma once

#include "toasts.hpp"

namespace houseofatmos {

    struct Balance {

        private:
        u64 coins = 0;

        public:
        void set_coins_silent(u64 amount) {
            this->coins = amount;
        }

        static inline const u64 infinite_coins = UINT64_MAX;
        static inline const u64 max_coins = UINT64_MAX - 1;

        void add_coins_silent(u64 amount) {
            if(this->coins == infinite_coins) { return; }
            u64 remaining_space = max_coins - this->coins;
            if(amount >= remaining_space) {
                this->coins = max_coins;
            } else {
                this->coins += amount;
            }
        }

        void add_coins(u64 amount, Toasts& toasts) {
            this->add_coins_silent(amount);
            toasts.add_toast("toast_added_coins", { std::to_string(amount) });
        }

        bool pay_coins_silent(u64 amount) {
            if(this->coins == infinite_coins) { return true; }
            if(this->coins < amount) { return false; }
            this->coins -= amount;
            return true;
        }

        bool pay_coins(u64 amount, Toasts& toasts) {
            bool paid = this->pay_coins_silent(amount);
            toasts.add_toast(
                paid? "toast_removed_coins" : "toast_too_expensive", 
                { std::to_string(amount) }
            );
            return paid;
        }

        ui::Element create_counter(ui::Element** handle_ptr) {
            std::string count_str = this->coins == infinite_coins? "∞"
                : std::to_string(this->coins);
            ui::Element counter = ui::Element()
                .with_handle(handle_ptr)
                .with_size(ui::width::text, ui::height::text)
                .with_text(count_str + " 🪙", &ui_font::dark)
                .with_padding(1.0)
                .with_pos(
                    ui::width::window - ui::unit * 15 - ui::horiz::width, 
                    ui::unit * 15
                )
                .with_background(&ui_background::note)
                .as_movable();
            return counter;
        }

        void update_counter(ui::Element& counter) {
            std::string count_str = this->coins == infinite_coins? "∞"
                : std::to_string(this->coins);
            counter.text = count_str + " 🪙";
        }
    };

}