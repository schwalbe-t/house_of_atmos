
#pragma once

#include "ui_const.hpp"

namespace houseofatmos::ui_util {

    using namespace houseofatmos::engine::math;
    namespace ui = houseofatmos::engine::ui;


    ui::Element create_selection_container(std::string title);

    ui::Element create_selection_item(
        const ui::Background* icon, std::string text, bool selected,
        std::function<void ()>&& handler
    );

    ui::Element create_text(
        std::string text, 
        f64 padding = 2.0,
        const ui::Font* font = &ui_font::dark
    );

    ui::Element create_button(
        std::string text, 
        std::function<void ()>&& handler = []() {},
        f64 padding = 2.0,
        f64 text_padding = 2.0,
        const ui::Font* font = &ui_font::bright,
        const ui::Background* background = &ui_background::button,
        const ui::Background* background_hover = &ui_background::button_select
    );

    ui::Element create_button(
        std::string text, 
        std::function<void (ui::Element&, Vec<2>)>&& handler,
        f64 padding = 2.0,
        f64 text_padding = 2.0,
        const ui::Font* font = &ui_font::bright,
        const ui::Background* background = &ui_background::button,
        const ui::Background* background_hover = &ui_background::button_select
    );

    ui::Element create_wide_button(
        std::string text, 
        std::function<void ()>&& handler = []() {},
        f64 padding = 2.0,
        f64 text_padding = 2.0,
        const ui::Font* font = &ui_font::bright,
        const ui::Background* background = &ui_background::button,
        const ui::Background* background_hover = &ui_background::button_select
    );

    ui::Element create_wide_button(
        std::string text, 
        std::function<void (ui::Element&, Vec<2>)>&& handler,
        f64 padding = 2.0,
        f64 text_padding = 2.0,
        const ui::Font* font = &ui_font::bright,
        const ui::Background* background = &ui_background::button,
        const ui::Background* background_hover = &ui_background::button_select
    );

    ui::Element create_icon(const ui::Background* icon);

    ui::Element create_icon_with_text(
        const ui::Background* icon, 
        std::string text,
        f64 padding = 2.0,
        f64 text_padding = 2.0, 
        const ui::Font* font = &ui_font::dark
    );

}