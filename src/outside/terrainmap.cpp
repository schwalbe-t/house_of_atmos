
#include "../ui_const.hpp"
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
            .as_phantom()
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
        this->selected_complex = std::nullopt;
        this->selected_carriage = std::nullopt;
        this->adding_stop = false;
        this->render_map();
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


    void TerrainMap::hide() {
        this->container->hidden = true;
        this->selected_info_right->hidden = true;
        this->selected_info_bottom->hidden = true;
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
        if(this->ui.is_hovered_over()) { return; }
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
        bool add_to_carriage = this->selected_carriage.has_value()
            && this->adding_stop
            && building != nullptr
            && building->complex.has_value();
        if(add_to_carriage) {
            Carriage& carriage = this->carriages
                .carriages[*this->selected_carriage];
            Carriage::Target target;
            target.complex = *building->complex;
            target.action = Carriage::LoadFixed;
            target.amount.fixed = 0;
            target.item = (Item::Type) 0;
            carriage.targets.push_back(target);
            this->adding_stop = false;
            return;
        } 
        this->selected_info_bottom->hidden = true;
        this->selected_info_right->hidden = true;
        this->selected_complex = std::nullopt;
        this->selected_carriage = std::nullopt;
        this->adding_stop = false;
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
        }
        this->render_map();
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
        if(this->selected_carriage.has_value()) {
            Carriage& carriage = this->carriages
                .carriages[*this->selected_carriage];
            *this->selected_info_right = this->display_carriage_info(carriage);
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

    static ui::Element create_single_marker_info(
        const ui::Background* icon, std::string text
    ) {
        Vec<2> icon_size = icon->edge_size;
        ui::Element info = ui::Element()
            .as_phantom()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Horizontal)
            .with_child(ui::Element()
                .as_phantom()
                .with_size(icon_size.x(), icon_size.y(), ui::size::units)
                .with_background(icon)
                .as_movable()
            )
            .with_child(ui::Element()
                .as_phantom()
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_pos(
                    0, (icon_size.y() - ui_font::bright.height) / 2 - 2.0, 
                    ui::position::parent_list_units
                )
                .with_text(text, &ui_font::dark)
                .with_padding(2)
                .as_movable()
            )
            .as_movable();
        return info;
    }

    void TerrainMap::create_marker_info() {
        this->container->children.push_back(ui::Element()
            .as_phantom()
            .with_pos(0.05, 0.95, ui::position::parent_offset_fract)
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Vertical)
            .with_child(create_single_marker_info(
                &ui_icon::map_marker_player, 
                this->local->text("ui_current_position")
            ))
            .with_child(create_single_marker_info(
                &ui_icon::map_marker_personal_horse,
                this->local->text("ui_personal_horse")
            ))
            .with_child(create_single_marker_info(
                &ui_icon::map_marker_carriage, 
                this->local->text("ui_carriage")
            ))
            .as_movable()
        );
    }

    void TerrainMap::add_marker(Vec<2> pos, ui::Element&& element) {
        Vec<2> pos_norm = pos 
            / this->terrain.units_per_tile()
            / Vec<2>(this->t_width, this->t_height);
        Vec<2> pos_px = this->container->final_pos()
            + this->view_pos_px 
            + pos_norm * this->view_size_px;
        Vec<2> pos_un = pos_px / this->ui.px_per_unit();
        this->container->children.push_back(ui::Element()
            .as_phantom()
            .with_pos(pos_un.x(), pos_un.y(), ui::position::window_tl_units)
            .with_size(0, 0, ui::size::units)
            .with_child(element
                .with_pos(0.5, 1.0, ui::position::parent_offset_fract)
                .as_movable()
            )
            .as_movable()
        );
    }

    void TerrainMap::add_icon_marker(
        Vec<2> pos, const ui::Background* icon, std::function<void ()>&& handler,
        bool is_phantom
    ) {
        this->add_marker(pos, ui::Element()
            .with_size(icon->edge_size.x(), icon->edge_size.y(), ui::size::units)
            .with_background(icon, &ui_icon::map_marker_selected)
            .with_click_handler(std::move(handler))
            .as_phantom(is_phantom)
            .as_movable()
        );
    }

    void TerrainMap::create_markers() {
        if(this->container == nullptr) { return; }
        if(this->container->hidden) { return; }
        this->container->children.clear();
        for(u64 carr_i = 0; carr_i < this->carriages.carriages.size(); carr_i += 1) {
            Carriage* carriage = &this->carriages.carriages[carr_i];
            bool is_selected = this->selected_carriage.has_value()
                && *this->selected_carriage == carr_i;
            this->add_icon_marker(
                carriage->position.swizzle<2>("xz"),
                carriage->is_lost()
                    ? &ui_icon::map_marker_agent_lost
                    : is_selected
                        ? &ui_icon::map_marker_selected
                        : &ui_icon::map_marker_carriage,
                [this, carriage, carr_i]() {
                    this->selected_info_bottom->hidden = true;
                    *this->selected_info_right = this->display_carriage_info(
                        *carriage
                    );
                    this->selected_complex = std::nullopt;
                    this->selected_carriage = carr_i;
                    this->render_map();
                }, false
            );
        }
        if(this->selected_carriage.has_value()) {
            const Carriage& carriage = this->carriages
                .carriages[*this->selected_carriage];
            u64 carr_x = (u64) (
                carriage.position.x() / this->terrain.units_per_tile()
            );
            u64 carr_z = (u64) (
                carriage.position.z() / this->terrain.units_per_tile()
            );
            for(size_t tgt_i = 0; tgt_i < carriage.targets.size(); tgt_i += 1) {
                const Complex& complex = this->complexes
                    .get(carriage.targets[tgt_i].complex);
                const auto& [mem_x, mem_z] = complex
                    .closest_member_to(carr_x, carr_z);
                const Building* member = this->terrain
                    .building_at((i64) mem_x, (i64) mem_z);
                assert(member != nullptr);
                const Building::TypeInfo& building = member->get_type_info();
                Vec<2> marker_pos = Vec<2>(
                    mem_x + building.width / 2.0, mem_z + building.height / 2.0
                ) * this->terrain.units_per_tile();
                this->add_marker(marker_pos, ui::Element()
                    .as_phantom()
                    .with_size(0, 0, ui::size::unwrapped_text)
                    .with_text(
                        "#" + std::to_string(tgt_i + 1), &ui_font::dark
                    )
                    .as_movable()
                );
            }
        }
        this->add_icon_marker(
            this->personal_horse.position().swizzle<2>("xz"), 
            &ui_icon::map_marker_personal_horse,
            [](){}, true
        );
        this->add_icon_marker(
            this->player.position.swizzle<2>("xz"), 
            &ui_icon::map_marker_player,
            [](){}, true
        );
    }

    void TerrainMap::render() {
        this->render_view();
        this->create_markers();
        this->create_marker_info();
    }



    ui::Element TerrainMap::display_item_stack(
        Item::Type type, const std::string& count, 
        const engine::Localization& local, f64* text_v_pad_out
    ) {
        const Item::TypeInfo& item = Item::items.at((size_t) type);
        std::string text = count + " "
            + local.text(item.local_name) + " ";
        f64 txt_pad = (item.icon->edge_size.y() - ui_font::dark.height) / 2;
        if(text_v_pad_out != nullptr) { *text_v_pad_out = txt_pad; }
        ui::Element info = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Horizontal)
            .with_child(ui::Element()
                .with_pos(0, txt_pad, ui::position::parent_list_units)
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(text, &ui_font::dark)
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
                        .with_pos(0, text_v_pad, ui::position::parent_list_units)
                        .with_size(0, 0, ui::size::unwrapped_text)
                        .with_text(" + ", &ui_font::dark)
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
                    .with_pos(0, text_v_pad, ui::position::parent_list_units)
                    .with_size(0, 0, ui::size::unwrapped_text)
                    .with_text(" → ", &ui_font::dark)
                    .as_movable()
                )
                .as_movable()
            )
            .with_child(std::move(outputs))
            .with_child(ui::Element()
                .with_size(0, 0, ui::size::units_with_children)
                .with_child(ui::Element()
                    .with_pos(0, text_v_pad, ui::position::parent_list_units)
                    .with_size(0, 0, ui::size::unwrapped_text)
                    .with_text(period_str, &ui_font::dark)
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
                            local.text(building.local_name), &ui_font::dark
                        )
                        .with_padding(1)
                        .as_movable()
                    )
                    .with_child(ui::Element()
                        .with_size(0, 0, ui::size::unwrapped_text)
                        .with_text(worker_info, &ui_font::dark)
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
        if(storage.children.size() == 0) {
            storage.children.push_back(ui::Element()
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(local.text("ui_empty"), &ui_font::dark)
                .as_movable()
            );
        }
        ui::Element info = ui::Element()
            .with_pos(0.95, 0.5, ui::position::window_fract)
            .with_size(0, 0, ui::size::units_with_children)
            .with_background(&ui_background::note)
            .with_list_dir(ui::Direction::Vertical)
            .with_child(ui::Element()
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(local.text("ui_building_complex"), &ui_font::dark)
                .with_padding(1)
                .as_movable()
            )
            .with_child(ui::Element()
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(local.text("ui_inputs"), &ui_font::dark)
                .with_padding(1)
                .as_movable()
            )
            .with_child(inputs.with_padding(3.0).as_movable())
            .with_child(ui::Element()
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(local.text("ui_outputs"), &ui_font::dark)
                .with_padding(1)
                .as_movable()
            )
            .with_child(outputs.with_padding(3.0).as_movable())
            .with_child(ui::Element()
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(local.text("ui_storage"), &ui_font::dark)
                .with_padding(1)
                .as_movable()
            )
            .with_child(storage.with_padding(3.0).as_movable())
            .as_movable();
        return info;
    }

    ui::Element TerrainMap::create_selection_container(std::string title) {
        ui::Element selector = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_background(&ui_background::note)
            .with_list_dir(ui::Direction::Vertical)
            .as_movable();
        if(title.size() > 0) {
            selector.children.push_back(ui::Element()
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(title, &ui_font::dark)
                .with_padding(2)
                .as_movable()
            );
        }
        return selector;
    }

    ui::Element TerrainMap::create_selection_item(
        const ui::Background* icon, std::string text, bool selected,
        std::function<void ()>&& handler
    ) {
        ui::Element item = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Horizontal)
            .with_child(ui::Element()
                .as_phantom()
                .with_size(
                    icon->edge_size.x(), icon->edge_size.y(), ui::size::units
                )
                .with_background(icon)
                .as_movable()
            )
            .with_child(ui::Element()
                .as_phantom()
                .with_pos(
                    0, 
                    (icon->edge_size.y() - ui_font::dark.height) / 2.0 - 2.0, 
                    ui::position::parent_list_units
                )
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(text, &ui_font::dark)
                .with_padding(2)
                .as_phantom()
                .as_movable()
            )
            .with_background(
                selected
                    ? &ui_background::border_selected
                    : &ui_background::border,
                selected
                    ? &ui_background::border_selected
                    : &ui_background::border_hovering
            )
            .with_click_handler(std::move(handler))
            .with_padding(2)
            .as_movable();
        return item;
    }

    ui::Element TerrainMap::display_item_selector(
        std::span<const Item::Type> items, 
        std::function<void (Item::Type)>&& passed_handler,
        const engine::Localization& local
    ) {
        std::shared_ptr<std::function<void (Item::Type)>> handler
            = std::make_shared<std::function<void (Item::Type)>>(
                std::move(passed_handler)
            );
        ui::Element container = TerrainMap::create_selection_container("")
            .with_pos(0.5, 0.5, ui::position::window_fract)
            .as_movable();
        for(Item::Type item: items) {
            const Item::TypeInfo& item_info = Item::items.at((size_t) item);
            container.children.push_back(TerrainMap::create_selection_item(
                item_info.icon, local.text(item_info.local_name), false,
                [item, handler]() { (*handler)(item); }
            ));
        }
        return container;
    }

    static ui::Element make_ui_button(std::string text) {
        ui::Element button = ui::Element()
            .as_phantom()
            .with_size(0, 0, ui::size::unwrapped_text)
            .with_text(text, &ui_font::bright)
            .with_padding(1)
            .with_background(
                &ui_background::button, &ui_background::button_select
            )
            .as_movable();
        return button;
    }

    ui::Element TerrainMap::display_carriage_target(
        Carriage* carriage, size_t target_i
    ) {
        Carriage::Target* target = &carriage->targets[target_i];
        std::string local_action = "";
        switch(target->action) {
            case Carriage::LoadFixed: case Carriage::LoadPercentage:
                local_action = "ui_pick_up"; break;
            case Carriage::PutFixed: case Carriage::PutPercentage:
                local_action = "ui_drop_off"; break;
        }
        std::string amount = "";
        std::string unit = "";
        switch(target->action) {
            case Carriage::LoadFixed: case Carriage::PutFixed:
                amount = std::to_string(target->amount.fixed);
                unit = "x";
                break;
            case Carriage::LoadPercentage: case Carriage::PutPercentage:
                amount = std::to_string(
                    (u64) round(target->amount.percentage * 100.0)
                );
                unit = "%";
                break;
        }
        std::string local_item = Item::items
            .at((size_t) target->item).local_name;
        ui::Element info = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Horizontal)
            .with_child(make_ui_button("🗑")
                .with_click_handler([carriage, target_i, this]() {
                    carriage->targets.erase(carriage->targets.begin() + target_i);
                    carriage->wrap_around_target_i();
                    carriage->try_find_path();
                })
                .with_padding(2)
                .as_movable()
            )
            .with_child(make_ui_button("↑")
                .with_click_handler([carriage, target_i]() {
                    size_t swapped_with_i = target_i == 0
                        ? carriage->targets.size() - 1
                        : target_i - 1;
                    std::swap(
                        carriage->targets[swapped_with_i],
                        carriage->targets[target_i]
                    );
                    carriage->clear_path();
                })
                .with_padding(2)
                .as_movable()
            )
            .with_child(make_ui_button("↓")
                .with_click_handler([carriage, target_i]() {
                    size_t swapped_with_i = (target_i + 1)
                        % carriage->targets.size();
                    std::swap(
                        carriage->targets[swapped_with_i],
                        carriage->targets[target_i]
                    );
                    carriage->clear_path();
                })
                .with_padding(2)
                .as_movable()
            )
            .with_child(ui::Element()
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(
                    "#" + std::to_string(target_i + 1), &ui_font::dark
                )
                .with_padding(3)
                .as_movable()
            )
            .with_child(make_ui_button(this->local->text(local_action))
                .with_click_handler([target]() {
                    switch(target->action) {
                        case Carriage::LoadFixed:
                            target->action = Carriage::PutFixed; break;
                        case Carriage::PutFixed:
                            target->action = Carriage::LoadFixed; break;
                        case Carriage::LoadPercentage:
                            target->action = Carriage::PutPercentage; break;
                        case Carriage::PutPercentage:
                            target->action = Carriage::LoadPercentage; break;
                    }
                })
                .with_padding(2)
                .as_movable()
            )
            .with_child(make_ui_button("+")
                .with_click_handler([target]() {
                    switch(target->action) {
                        case Carriage::LoadFixed: case Carriage::PutFixed:
                            target->amount.fixed += 1;
                            break;
                        case Carriage::LoadPercentage: case Carriage::PutPercentage:
                            target->amount.percentage = std::min(
                                (i64) round(target->amount.percentage / 0.05) + 1, 
                                (i64) round(100.0 / 0.05)
                            ) * 0.05;
                            break;
                    }
                })
                .with_padding(2)
                .as_movable()
            )
            .with_child(make_ui_button("-")
                .with_click_handler([target]() {
                    switch(target->action) {
                        case Carriage::LoadFixed: case Carriage::PutFixed:
                            if(target->amount.fixed > 0) {
                                target->amount.fixed -= 1;
                            }
                            break;
                        case Carriage::LoadPercentage: case Carriage::PutPercentage:
                            target->amount.percentage = std::max(
                                (i64) round(target->amount.percentage / 0.05) - 1, 
                                (i64) 0
                            ) * 0.05;
                            break;
                    }
                })
                .with_padding(2)
                .as_movable()
            )
            .with_child(ui::Element()
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(amount, &ui_font::dark)
                .with_padding(3)
                .as_movable()
            )
            .with_child(make_ui_button(unit)
                .with_click_handler([target]() {
                    target->amount.fixed = 0; // also sets percentage to 0
                    switch(target->action) {
                        case Carriage::LoadFixed:
                            target->action = Carriage::LoadPercentage; break;
                        case Carriage::PutFixed:
                            target->action = Carriage::PutPercentage; break;
                        case Carriage::LoadPercentage:
                            target->action = Carriage::LoadFixed; break;
                        case Carriage::PutPercentage:
                            target->action = Carriage::PutFixed; break;
                    }
                })
                .with_padding(2)
                .as_movable()
            )
            .with_child(make_ui_button(this->local->text(local_item))
                .with_click_handler([this, target]() {
                    this->adding_stop = false;
                    *this->selected_info_bottom 
                        = TerrainMap::display_item_selector(
                            Item::transferrable, [this, target](auto selected) {
                                target->item = selected;
                                *this->selected_info_bottom = ui::Element()
                                    .as_phantom().as_movable();
                            }, *this->local
                        )
                        .with_pos(0.5, 0.5, ui::position::window_fract)
                        .as_movable();
                })
                .with_padding(2)
                .as_movable()
            )
            .as_movable();
        return info;
    }

    ui::Element TerrainMap::display_carriage_info(Carriage& carriage) {
        std::string status = "ui_status_no_instructions";
        if(carriage.target_i().has_value()) {
            switch(carriage.current_state()) {
                case Carriage::State::Loading: 
                    status = "ui_status_transferring"; break;
                case Carriage::State::Travelling: 
                    status = "ui_status_travelling"; break;
                case Carriage::State::Lost: 
                    status = "ui_status_lost"; break;
            }
        }
        std::string status_text = this->local->pattern(
            status, { std::to_string(*carriage.target_i() + 1) }
        );
        ui::Element storage = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Vertical)
            .as_movable();
        for(const auto& [item, count]: carriage.stored_items()) {
            if(count == 0) { continue; }
            storage.children.push_back(TerrainMap::display_item_stack(
                item, std::to_string(count), *this->local
            ));
        }
        if(storage.children.size() == 0) {
            storage.children.push_back(ui::Element()
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(this->local->text("ui_empty"), &ui_font::dark)
                .as_movable()
            );
        }
        ui::Element schedule = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Vertical)
            .as_movable();
        for(u64 target_i = 0; target_i < carriage.targets.size(); target_i += 1) {
            schedule.children.push_back(
                this->display_carriage_target(&carriage, target_i)
            );
        }
        if(schedule.children.size() == 0) {
            schedule.children.push_back(ui::Element()
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(this->local->text("ui_empty"), &ui_font::dark)
                .as_movable()
            );
        }
        ui::Element add_stop = this->adding_stop
            ? ui::Element()
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(
                    this->local->text("ui_click_target_complex"), 
                    &ui_font::dark
                )
                .with_padding(4)
                .as_movable()
            : make_ui_button(this->local->text("ui_add_stop"))
                .with_click_handler([this]() {
                    this->adding_stop = true;
                })
                .with_padding(4)
                .as_movable();
        ui::Element info = ui::Element()
            .with_pos(0.95, 0.5, ui::position::window_fract)
            .with_size(0, 0, ui::size::units_with_children)
            .with_background(&ui_background::note)
            .with_list_dir(ui::Direction::Vertical)
            .with_child(ui::Element()
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(this->local->text("ui_carriage"), &ui_font::dark)
                .with_padding(1)
                .as_movable()
            )
            .with_child(ui::Element()
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(status_text, &ui_font::dark)
                .with_padding(3)
                .as_movable()
            )
            .with_child(make_ui_button(this->local->text("ui_remove_carriage"))
                .with_click_handler([this]() {
                    auto selected = this->carriages.carriages.begin() 
                        + *this->selected_carriage;
                    this->carriages.carriages.erase(selected);
                    this->selected_info_right->hidden = true;
                    this->selected_info_bottom->hidden = true;
                    this->selected_carriage = std::nullopt;
                    this->adding_stop = false;
                })
                .with_padding(4)
                .as_movable()
            )
            .with_child(ui::Element()
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(this->local->text("ui_schedule"), &ui_font::dark)
                .with_padding(1)
                .as_movable()
            )
            .with_child(schedule.with_padding(3.0).as_movable())
            .with_child(std::move(add_stop))
            .with_child(ui::Element()
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(this->local->text("ui_on_board"), &ui_font::dark)
                .with_padding(1)
                .as_movable()
            )
            .with_child(storage.with_padding(3.0).as_movable())
            .as_movable();
        return info;
    }

}