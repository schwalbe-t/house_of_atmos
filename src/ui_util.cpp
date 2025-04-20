
#include "ui_util.hpp"

namespace houseofatmos::ui_util {

    ui::Element create_selection_container(std::string title) {
        ui::Element selector = ui::Element()
            .with_background(&ui_background::note)
            .with_list_dir(ui::Direction::Vertical)
            .as_movable();
        if(title.size() > 0) {
            selector.children.push_back(create_text(title));
        }
        return selector;
    }


    ui::Element create_selection_item(
        const ui::Background* icon, std::string text, bool selected,
        std::function<void ()>&& handler
    ) {
        ui::LayoutFunc pad_r_func = ui::unit * 2;
        ui::LayoutFunc pad_d_func = ui::unit * 4;
        ui::Element item = create_icon_with_text(icon, text, 0.0)
            .with_background(
                selected
                    ? &ui_background::border_selected
                    : &ui_background::border,
                selected
                    ? &ui_background::border_selected
                    : &ui_background::border_hovering
            )
            .with_click_handler(std::move(handler))
            .as_movable();
        item.width_func = item.width_func.max(ui::width::parent - pad_d_func);
        item.horiz_func = ui::horiz::parent + pad_r_func;
        item.vert_func = ui::vert::parent + pad_r_func;
        ui::Element container = ui::Element().as_phantom().as_movable();
        container.width_func = ui::width::parent
            .max(ui::width::children + pad_d_func);
        container.height_func = ui::height::children + pad_d_func;
        container.children.push_back(std::move(item));
        return container;
    }


    ui::Element create_text(
        std::string text,
        f64 padding,
        const ui::Font* font
    ) {
        ui::Element span = ui::Element()
            .as_phantom()
            .with_size(ui::width::text, ui::height::text)
            .with_text(text, font)
            .with_padding(padding)
            .as_phantom()
            .as_movable();
        return span;
    }


    ui::Element create_button(
        std::string text, 
        std::function<void (ui::Element&, Vec<2>)>&& handler,
        f64 padding,
        f64 text_padding,
        const ui::Font* font,
        const ui::Background* background,
        const ui::Background* background_hover
    ) {
        ui::Element button = create_text(text, text_padding, font)
            .as_phantom(false)
            .with_background(background, background_hover)
            .with_click_handler(std::move(handler))
            .with_padding(padding)
            .as_phantom()
            .as_movable();
        return button;
    }

    ui::Element create_button(
        std::string text, 
        std::function<void ()>&& handler,
        f64 padding,
        f64 text_padding,
        const ui::Font* font,
        const ui::Background* background,
        const ui::Background* background_hover
    ) {
        return create_button(
            text,
            [handler](auto& e, auto c) {
                (void) e;
                (void) c;
                handler();
            },
            padding, text_padding,
            font, background, background_hover
        );
    }

    ui::Element create_wide_button(
        std::string text, 
        std::function<void (ui::Element&, Vec<2>)>&& handler,
        f64 padding,
        f64 text_padding,
        const ui::Font* font,
        const ui::Background* background,
        const ui::Background* background_hover
    ) {
        ui::LayoutFunc pad_r_func = ui::unit * padding;
        ui::LayoutFunc pad_d_func = ui::unit * (padding * 2);
        ui::Element button = create_text(text, text_padding, font)
            .as_phantom(false)
            .with_background(background, background_hover)
            .with_click_handler(std::move(handler))
            .as_movable();
        button.horiz_func = ui::horiz::parent + pad_r_func;
        button.vert_func = ui::vert::parent + pad_r_func;
        button.width_func = button.width_func
            .max(ui::width::parent - pad_d_func);
        ui::Element container = ui::Element()
            .as_phantom()
            .as_movable();
        container.width_func = (ui::width::children + pad_d_func)
            .max(ui::width::parent);
        container.height_func = ui::height::children + pad_d_func;
        container.children.push_back(std::move(button));
        return container;
    }

    ui::Element create_wide_button(
        std::string text, 
        std::function<void ()>&& handler,
        f64 padding,
        f64 text_padding,
        const ui::Font* font,
        const ui::Background* background,
        const ui::Background* background_hover
    ) {
        return create_wide_button(
            text,
            [handler](auto& e, auto c) {
                (void) e;
                (void) c;
                handler();
            },
            padding, text_padding,
            font, background, background_hover
        );
    }


    ui::Element create_icon(const ui::Background* icon) {
        ui::Element result = ui::Element()
            .as_phantom()
            .with_size(
                ui::unit * icon->edge_size.x(), ui::unit * icon->edge_size.y()
            )
            .with_background(icon)
            .as_movable();
        return result;
    }


    ui::Element create_icon_with_text(
        const ui::Background* icon, 
        std::string text,
        f64 padding,
        f64 text_padding, 
        const ui::Font* font
    ) {
        f64 text_vert_pad = (icon->edge_size.y() - font->height) / 2 
            - text_padding;
        ui::Element root = ui::Element()
            .as_phantom()
            .with_list_dir(ui::Direction::Horizontal)
            .as_movable();
        root.children.push_back(create_icon(icon));
        root.children.push_back(create_text(text, text_padding, font)
            .with_pos(
                ui::horiz::list, ui::vert::list + ui::unit * text_vert_pad
            )
            .as_movable()
        );
        ui::Element padded = root.with_padding(padding).as_movable();
        return padded;
    }

}