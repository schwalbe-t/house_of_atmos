
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
    };

}