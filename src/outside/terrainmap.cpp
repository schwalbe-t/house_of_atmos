
#include "../ui_font.hpp"
#include "terrainmap.hpp"
#include <format>

namespace houseofatmos::outside {

    using Color = engine::Image::Color;


    static const Color high_grass_color = Color(192, 199, 65);
    static const Color low_grass_color = Color(98, 122, 51);
    static const Color sand_color = Color(245, 237, 186);
    static const Color high_water_color = Color(110, 169, 167);
    static const Color low_water_color = Color(44, 101, 118); 
    static const Color path_color = Color(154, 99, 72);
    static const Color building_color = Color(157, 48, 59);
    static const Color background_color = Color(255, 202, 168);

    static const f64 step_size = 5;
    static const f64 grass_high = 25;
    static const f64 water_level = 0.0;
    static const f64 water_low = -25;
    static const f64 fadeout_dist = 5;

    static Color color_lerp(Color a, Color b, f64 n) {
        if(n < 0.0) { n = 0.0; }
        if(n > 1.0) { n = 1.0; }
        return Color(
            a.r + (u8) (n * (b.r - a.r)),
            a.g + (u8) (n * (b.g - a.g)),
            a.b + (u8) (n * (b.b - a.b)),
            a.a + (u8) (n * (b.a - a.a)) 
        );
    }

    ui::Element TerrainMap::create_container() {
        ui::Element container = ui::Element()
            .with_pos(0.5, 0.5, ui::position::window_fract)
            .with_size(0.9, 0.9, ui::size::window_fract)
            .with_background(&ui_background::scroll_horizontal)
            .with_handle(&this->container)
            .as_hidden(true)
            .as_movable();
        return container;
    }

    void TerrainMap::render_map() {
        for(u64 x = 0; x < this->t_width; x += 1) {
            for(u64 z = 0; z < this->t_height; z += 1) {
                // get elevation data
                i64 elev_tl = this->terrain.elevation_at(x,     z    );
                i64 elev_tr = this->terrain.elevation_at(x + 1, z    );
                i64 elev_bl = this->terrain.elevation_at(x,     z + 1);
                i64 elev_br = this->terrain.elevation_at(x + 1, z + 1);
                f64 elev_avg = (elev_tl + elev_tr + elev_bl + elev_br) / 4.0;
                f64 step = trunc(elev_avg / step_size) * step_size;
                // compute pixel color
                Color grass_color = color_lerp(
                    low_grass_color, high_grass_color,
                    (step - water_level) / grass_high
                );
                Color water_color = color_lerp(
                    high_water_color, low_water_color,
                    (step - water_level) / water_low
                );
                Color color = grass_color;
                // if any vertex below water level, sand
                bool is_sand = elev_tl < 0 || elev_tr < 0
                    || elev_bl < 0 || elev_br < 0;
                if(is_sand) { color = sand_color; }
                // if all vertices below water level, water
                bool is_water = elev_tl < 0 && elev_tr < 0 
                    && elev_bl < 0 && elev_br < 0;
                if(is_water) { color = water_color; }
                // if is path or building, make building
                bool is_path = this->terrain.path_at((i64) x, (i64) z);
                if(is_path) { color = path_color; }
                bool is_building = this->terrain.building_at(x, z) != nullptr;
                if(is_building) { color = building_color; }
                // fade to background color towards edges (hack, should be alpha)
                u64 edge_dist = std::min(
                    std::min(x + 1, this->t_width - x),
                    std::min(z + 1, this->t_height - z)
                );
                f64 edge_fadeout = std::min(edge_dist / fadeout_dist, 1.0);
                color = color_lerp(background_color, color, edge_fadeout);
                // write computed color
                this->rendered_img.pixel_at(x, z) = color;
            }
        }
        this->rendered_tex = std::move(engine::Texture(this->rendered_img));
    }


    bool TerrainMap::toggle_with_key(
        engine::Key key, const engine::Window& window
    ) {
        if(window.was_pressed(key)) {
            this->container->hidden = !this->container->hidden;
            if(!this->container->hidden) {
                this->render_map();
            }
            return !this->container->hidden;
        }
        return false;
    }

    void TerrainMap::update(const engine::Window& window) {
        if(this->container == nullptr) { return; }
        if(this->container->hidden) { return; }
        f64 view_h = (f64) this->container->final_size().y() * this->view_scale;
        f64 view_w = (f64) this->t_width / this->t_height * view_h;
        Vec<2> view_size = Vec<2>(view_w, view_h);
        // zoom camera
        if(window.scrolled().y() != 0.0) {
            Vec<2> cursor_diff = (window.cursor_pos_px() 
                - this->container->final_pos() 
                - (this->container->final_size() / 2)
            ) / view_size;
            f64 new_scale = this->view_scale + window.scrolled().y() * 0.1;
            new_scale = std::min(std::max(new_scale, 0.5), 2.0);
            this->view_pos += cursor_diff * (1.0 - this->view_scale / new_scale);
            this->view_scale = new_scale;
        }
        // move camera
        if(window.is_down(engine::Button::Left)) {
            if(this->view_anchor.has_value()) {
                Vec<2> diff = *this->view_anchor - window.cursor_pos_px();
                this->view_pos += diff / view_size;
            }
            this->view_anchor = window.cursor_pos_px();
        } else {
            this->view_anchor = std::nullopt;
        }
        this->view_pos.x() = std::min(std::max(this->view_pos.x(), 0.0), 1.0);
        this->view_pos.y() = std::min(std::max(this->view_pos.y(), 0.0), 1.0);
    }

    void TerrainMap::render_view() {
        if(this->container == nullptr) { return; }
        if(this->container->hidden) { return; }
        if(this->container->final_size().min() <= 0) { return; }
        this->output_tex.resize_fast(
            (u64) this->container->final_size().x(), 
            (u64) this->container->final_size().y()
        );
        this->output_tex.clear_color(Vec<4>(0, 0, 0, 0));
        f64 view_h = (f64) this->output_tex.height() * this->view_scale;
        f64 view_w = (f64) this->t_width / this->t_height * view_h;
        f64 view_x = this->output_tex.width() / 2 
            - view_w * this->view_pos.x();
        f64 view_y = this->output_tex.height() / 2 
            - view_h * this->view_pos.y();
        this->rendered_tex.blit(this->output_tex, view_x, view_y, view_w, view_h);
        this->container->texture = &this->output_tex;
    }



    ui::Element TerrainMap::display_item_stack(
        const Item::Stack& stack, const engine::Localization& local,
        f64* text_v_pad_out
    ) {
        const Item::TypeInfo& item = Item::items.at((size_t) stack.item);
        std::string text = std::to_string(stack.count) + " "
            + local.text(item.local_name) + " ";
        f64 txt_pad = (item.icon->edge_size.y() - ui_font::standard.height) / 2;
        if(text_v_pad_out != nullptr) { *text_v_pad_out = txt_pad; }
        ui::Element info = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Horizontal)
            .with_child(ui::Element()
                .with_pos(0, txt_pad, ui::position::parent_units)
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(text, &ui_font::standard)
                .as_movable()
            )
            .with_child(ui::Element()
                .with_size(
                    item.icon->edge_size.x(), item.icon->edge_size.y(), 
                    ui::size::units
                )
                .with_background(item.icon)
                .as_movable()
            )
            .as_movable();
        return info;
    }

    ui::Element TerrainMap::display_item_stack_list(
        std::span<const Item::Stack> stacks, 
        const engine::Localization& local,
        f64* text_v_pad_out
    ) {
        ui::Element list = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Horizontal)
            .as_movable();
        bool had_item = false;
        f64 text_v_pad = 0.0;
        for(const Item::Stack& stack: stacks) {
            if(had_item) {
                list.children.push_back(ui::Element()
                    .with_size(0, 0, ui::size::units_with_children)
                    .with_child(ui::Element()
                        .with_pos(0, text_v_pad, ui::position::parent_units)
                        .with_size(0, 0, ui::size::unwrapped_text)
                        .with_text(" + ", &ui_font::standard)
                        .as_movable()
                    )
                    .as_movable()
                );
            }
            list.children.push_back(
                TerrainMap::display_item_stack(stack, local, &text_v_pad)
            );
            had_item = true;
        }
        if(text_v_pad_out != nullptr) { *text_v_pad_out = text_v_pad; }
        return list;
    }

    ui::Element TerrainMap::display_conversion(
        const Conversion& conv, const engine::Localization& local
    ) {
        std::string period_str = std::format(" ({}s)", conv.period);
        f64 text_v_pad = 0;
        ui::Element info = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Horizontal)
            .with_child(TerrainMap::display_item_stack_list(
                conv.inputs, local, &text_v_pad
            ))
            .with_child(ui::Element()
                .with_size(0, 0, ui::size::units_with_children)
                .with_child(ui::Element()
                    .with_pos(0, text_v_pad, ui::position::parent_units)
                    .with_size(0, 0, ui::size::unwrapped_text)
                    .with_text(" → ", &ui_font::standard)
                    .as_movable()
                )
                .as_movable()
            )
            .with_child(TerrainMap::display_item_stack_list(
                conv.outputs, local, &text_v_pad
            ))
            .with_child(ui::Element()
                .with_size(0, 0, ui::size::units_with_children)
                .with_child(ui::Element()
                    .with_pos(0, text_v_pad, ui::position::parent_units)
                    .with_size(0, 0, ui::size::unwrapped_text)
                    .with_text(period_str, &ui_font::standard)
                    .as_movable()
                )
                .as_movable()
            )
            .as_movable();
        return info;
    }

    ui::Element TerrainMap::display_building_info(
        Building::Type type, std::span<const Conversion> conversions,
        const engine::Localization& local
    ) {
        const Building::TypeInfo& building = Building::types.at((size_t) type);
        std::string worker_info = building.residents > building.workers
            ? local.pattern(
                "ui_provided_workers", 
                { std::to_string(building.residents - building.workers) }
            )
            : local.pattern(
                "ui_required_workers",
                { std::to_string(building.workers - building.residents) }
            );
        ui::Element info = ui::Element()
            .with_pos(0.5, 0.95, ui::position::window_fract)
            .with_size(0, 0, ui::size::units_with_children)
            .with_background(&ui_background::note)
            .with_list_dir(ui::Direction::Vertical)
            .with_child(ui::Element()
                .with_size(0, 0, ui::size::units_with_children)
                .with_list_dir(ui::Direction::Horizontal)
                .with_child(ui::Element()
                    .with_size(
                        building.icon->edge_size.x(), building.icon->edge_size.y(),
                        ui::size::units
                    )
                    .with_background(building.icon)
                    .with_padding(2)
                    .as_movable()
                )
                .with_child(ui::Element()
                    .with_size(0, 0, ui::size::units_with_children)
                    .with_list_dir(ui::Direction::Vertical)
                    .with_child(ui::Element()
                        .with_size(0, 0, ui::size::unwrapped_text)
                        .with_text(
                            local.text(building.local_name), &ui_font::standard
                        )
                        .with_padding(1)
                        .as_movable()
                    )
                    .with_child(ui::Element()
                        .with_size(0, 0, ui::size::unwrapped_text)
                        .with_text(worker_info, &ui_font::standard)
                        .with_padding(1)
                        .as_movable()
                    )
                    .with_padding(1)
                    .as_movable()
                )
                .as_movable()
            )
            .as_movable();
        for(const Conversion& conv: conversions) {
            info.children.push_back(TerrainMap::display_conversion(conv, local));
        }
        return info;
    }

}