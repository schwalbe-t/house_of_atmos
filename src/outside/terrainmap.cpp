
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
    static const Color selected_complex_color = Color(239, 239, 230);

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

    void TerrainMap::create_container() {
        this->ui.root.children.push_back(ui::Element()
            .with_pos(0.5, 0.5, ui::position::window_fract)
            .with_size(0.9, 0.9, ui::size::window_fract)
            .with_background(&ui_background::scroll_horizontal)
            .with_handle(&this->container)
            .as_hidden(true)
            .as_movable()
        );
        this->ui.root.children.push_back(std::move(ui::Element().as_phantom()));
        this->selected_info_right = &this->ui.root.children.back();
        this->ui.root.children.push_back(std::move(ui::Element().as_phantom()));
        this->selected_info_bottom = &this->ui.root.children.back();
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
                const Building* building = this->terrain.building_at(x, z);
                if(building != nullptr) { color = building_color; }
                bool selected = building != nullptr
                    && building->complex.has_value()
                    && this->selected_complex.has_value()
                    && building->complex->index == this->selected_complex->index;
                if(selected) { color = selected_complex_color; }
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
            this->selected_info_right->hidden = this->container->hidden;
            this->selected_info_bottom->hidden = this->container->hidden;
            if(!this->container->hidden) {
                this->render_map();
            }
            return !this->container->hidden;
        }
        return false;
    }

    bool TerrainMap::hovering_marker() {
        for(const ui::Element& element: this->container->children) {
            if(element.is_hovered_over()) { return true; }
        }
        return false;
    }

    static const f64 min_zoom = 0.5;
    static const f64 max_zoom = 4.0;
    static const f64 zoom_step = 0.5;

    void TerrainMap::update_view(const engine::Window& window) {
        f64 view_h = (f64) this->container->final_size().y() * this->view_scale;
        f64 view_w = (f64) this->t_width / this->t_height * view_h;
        Vec<2> view_size = Vec<2>(view_w, view_h);
        // zoom camera
        if(window.scrolled().y() != 0.0) {
            Vec<2> cursor_diff = (window.cursor_pos_px() 
                - this->container->final_pos() 
                - (this->container->final_size() / 2)
            ) / view_size;
            f64 new_scale = this->view_scale + window.scrolled().y() * zoom_step;
            new_scale = std::min(std::max(new_scale, min_zoom), max_zoom);
            this->view_pos += cursor_diff * (1.0 - this->view_scale / new_scale);
            this->view_scale = new_scale;
        }
        // move camera
        bool move_down = window.is_down(engine::Button::Right)
            || window.is_down(engine::Button::Middle);
        if(move_down) {
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

    void TerrainMap::update_click(const engine::Window& window) {
        if(!window.was_pressed(engine::Button::Left)) { return; }
        if(this->hovering_marker()) { return; }
        Vec<2> rel_pos_px = window.cursor_pos_px() 
            - this->container->final_pos()
            - this->view_pos_px;
        Vec<2> pos = rel_pos_px 
            / this->view_size_px
            * Vec<2>(this->t_width, this->t_height);
        i64 x = (i64) pos.x();
        i64 z = (i64) pos.y();
        u64 b_chunk_x, b_chunk_z;
        const Building* building = this->terrain
            .building_at(x, z, &b_chunk_x, &b_chunk_z);
        if(building != nullptr) {
            std::span<const Conversion> conversions;
            if(building->complex.has_value()) {
                u64 ax = b_chunk_x * this->terrain.tiles_per_chunk() + building->x;
                u64 az = b_chunk_z * this->terrain.tiles_per_chunk() + building->z;
                const Complex& complex = this->complexes.get(*building->complex);
                const Complex::Member& member = complex.member_at(ax, az);
                conversions = member.conversions;
            }
            *this->selected_info_bottom = TerrainMap::display_building_info(
                building->type, conversions, *this->local
            );
            *this->selected_info_right = building->complex.has_value()
                ? TerrainMap::display_complex_info(
                    this->complexes.get(*building->complex), *this->local
                )
                : ui::Element().as_phantom().as_movable();
            this->selected_complex = building->complex;
            this->render_map();
        } else {
            this->selected_info_bottom->hidden = true;
            this->selected_info_right->hidden = true;
            this->selected_complex = std::nullopt;
            this->render_map();
        }
    }

    void TerrainMap::update(const engine::Window& window, engine::Scene& scene) {
        this->local = &scene.get<engine::Localization>(this->local_ref);
        if(this->container == nullptr) { return; }
        if(this->container->hidden) { return; }
        this->update_view(window);
        this->update_click(window);
        if(this->selected_complex.has_value()) {
            const Complex& complex = this->complexes.get(*this->selected_complex);
            *this->selected_info_right 
                = TerrainMap::display_complex_info(complex, *this->local);
        }
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
        this->view_size_px.x() = (f64) this->output_tex.height() 
            * this->view_scale;
        this->view_size_px.y() = (f64) this->t_width / this->t_height 
            * this->view_size_px.x();
        this->view_pos_px = Vec<2>(this->output_tex.width(), this->output_tex.height()) / 2
            - this->view_size_px * this->view_pos;
        this->rendered_tex.blit(
            this->output_tex, 
            this->view_pos_px.x(), this->view_pos_px.y(), 
            this->view_size_px.x(), this->view_size_px.y()
        );
        this->container->texture = &this->output_tex;
    }

    void TerrainMap::add_marker(
        Vec<2> pos, const ui::Background* icon, std::function<void ()>&& handler,
        bool is_phantom
    ) {
        Vec<2> pos_norm = pos 
            / this->terrain.units_per_tile()
            / Vec<2>(this->t_width, this->t_height);
        Vec<2> pos_px = this->container->final_pos()
            + this->view_pos_px 
            + pos_norm * this->view_size_px;
        Vec<2> pos_un = pos_px / this->ui.px_per_unit();
        this->container->children.push_back(ui::Element()
            .with_pos(
                pos_un.x() - icon->edge_size.x() / 2, 
                pos_un.y() - icon->edge_size.y(), 
                ui::position::window_tl_units
            )
            .with_size(icon->edge_size.x(), icon->edge_size.y(), ui::size::units)
            .with_background(icon)
            .with_click_handler(std::move(handler))
            .as_phantom(is_phantom)
            .as_movable()
        );
    }

    void TerrainMap::create_markers() {
        if(this->container == nullptr) { return; }
        if(this->container->hidden) { return; }
        this->container->children.clear();
        for(const Carriage& carriage: this->carriages.carriages) {
            this->add_marker(
                carriage.position.swizzle<2>("xz"),
                carriage.is_lost()
                    ? &ui_icon::map_marker_carriage_lost
                    : &ui_icon::map_marker_carriage,
                []() { engine::debug("not yet implemented :)"); }, false
            );
        }
        this->add_marker(
            this->player.position.swizzle<2>("xz"), &ui_icon::map_marker_player,
            [](){}, true
        );
    }

    void TerrainMap::render() {
        this->render_view();
        this->create_markers();
    }



    ui::Element TerrainMap::display_item_stack(
        Item::Type type, const std::string& count, 
        const engine::Localization& local, f64* text_v_pad_out
    ) {
        const Item::TypeInfo& item = Item::items.at((size_t) type);
        std::string text = count + " "
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
            list.children.push_back(TerrainMap::display_item_stack(
                stack.item, std::to_string(stack.count), local, &text_v_pad
            ));
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
        ui::Element inputs = TerrainMap::display_item_stack_list(
            conv.inputs, local, &text_v_pad
        );
        ui::Element outputs = TerrainMap::display_item_stack_list(
            conv.outputs, local, &text_v_pad
        );
        ui::Element info = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Horizontal)
            .with_child(std::move(inputs))
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
            .with_child(std::move(outputs))
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

    ui::Element TerrainMap::display_complex_info(
        const Complex& complex, const engine::Localization& local
    ) {
        std::unordered_map<Item::Type, f64> throughput 
            = complex.compute_throughput();
        ui::Element inputs = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Vertical)
            .as_movable();
        for(const auto& [item, freq]: throughput) {
            if(freq >= 0.0) { continue; }
            inputs.children.push_back(TerrainMap::display_item_stack(
                item, std::format("{}", fabs(freq)), local
            ));
        }
        ui::Element outputs = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Vertical)
            .as_movable();
        for(const auto& [item, freq]: throughput) {
            if(freq <= 0.0) { continue; }
            outputs.children.push_back(TerrainMap::display_item_stack(
                item, std::format("{}", freq), local
            ));
        }
        ui::Element storage = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Vertical)
            .as_movable();
        for(const auto& [item, count]: complex.stored_items()) {
            if(count == 0) { continue; }
            storage.children.push_back(TerrainMap::display_item_stack(
                item, std::to_string(count), local
            ));
        }
        ui::Element info = ui::Element()
            .with_pos(0.95, 0.5, ui::position::window_fract)
            .with_size(0, 0, ui::size::units_with_children)
            .with_background(&ui_background::note)
            .with_list_dir(ui::Direction::Vertical)
            .with_child(ui::Element()
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(local.text("ui_building_complex"), &ui_font::standard)
                .with_padding(1)
                .as_movable()
            )
            .with_child(ui::Element()
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(local.text("ui_inputs"), &ui_font::standard)
                .with_padding(1)
                .as_movable()
            )
            .with_child(inputs.with_padding(3.0).as_movable())
            .with_child(ui::Element()
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(local.text("ui_outputs"), &ui_font::standard)
                .with_padding(1)
                .as_movable()
            )
            .with_child(outputs.with_padding(3.0).as_movable())
            .with_child(ui::Element()
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(local.text("ui_storage"), &ui_font::standard)
                .with_padding(1)
                .as_movable()
            )
            .with_child(storage.with_padding(3.0).as_movable())
            .as_movable();
        return info;
    }

}