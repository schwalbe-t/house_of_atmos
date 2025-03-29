
#include "../ui_const.hpp"
#include "terrainmap.hpp"
#include <format>

namespace houseofatmos::world {

    using Color = engine::Image::Color;


    static const Color high_grass_color = Color(192, 199, 65);
    static const Color low_grass_color = Color(98, 122, 51);
    static const Color sand_color = Color(245, 237, 186);
    static const Color high_water_color = Color(110, 169, 167);
    static const Color low_water_color = Color(44, 101, 118); 
    static const Color resource_color = Color(88, 69, 99);
    static const Color path_color = Color(154, 99, 72);
    static const Color train_track_color = Color(140, 143, 174);
    static const Color building_color = Color(157, 48, 59);
    static const Color background_color = Color(84, 142, 148);
    static const Color selected_complex_color = Color(239, 239, 230);

    static const f64 step_size = 5;
    static const f64 grass_high = 25;
    static const f64 water_level = 0.0;
    static const f64 water_low = -25;

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
        this->selected_type = SelectionType::None;
        this->adding_stop = false;
        this->render_map();
    }

    void TerrainMap::render_map() {
        for(u64 x = 0; x < this->t_width; x += 1) {
            for(u64 z = 0; z < this->t_height; z += 1) {
                // get elevation data
                i64 elev_tl = this->world->terrain.elevation_at(x,     z    );
                i64 elev_tr = this->world->terrain.elevation_at(x + 1, z    );
                i64 elev_bl = this->world->terrain.elevation_at(x,     z + 1);
                i64 elev_br = this->world->terrain.elevation_at(x + 1, z + 1);
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
                // if is resource
                bool is_resource = this->world
                    ->terrain.resource_at((i64) x, (i64) z);
                if(is_resource) { color = resource_color; }
                // if is path
                bool is_path = this->world->terrain.path_at((i64) x, (i64) z)
                    || this->world->terrain.bridge_at((i64) x, (i64) z) != nullptr;
                if(is_path) { color = path_color; }
                // if is track
                bool is_track = this->world->terrain
                    .track_pieces_at((i64) x, (i64) z) > 0;
                if(is_track) { color = train_track_color; }
                // if is building
                const Building* building = this->world->terrain.building_at(x, z);
                if(building != nullptr) { color = building_color; }
                bool selected = building != nullptr
                    && building->complex.has_value()
                    && this->selected_type == SelectionType::Complex
                    && this->selected.complex.index == building->complex->index;
                if(selected) { color = selected_complex_color; }
                // write computed color
                this->rendered_img.pixel_at(x, z) = color;
            }
        }
        this->rendered_tex = engine::Texture(this->rendered_img);
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
            return true;
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
        const Building* building = this->world->terrain
            .building_at(x, z, &b_chunk_x, &b_chunk_z);
        bool add_to_agent = this->adding_stop
            && this->selected_type == SelectionType::Agent
            && building != nullptr
            && building->complex.has_value();
        if(add_to_agent) {
            AgentStop stop;
            stop.target = *building->complex;
            stop.action = AgentStop::Load;
            stop.amount.fract = 1.0;
            stop.unit = AgentStop::Fraction;
            stop.item = (Item::Type) 0;
            this->selected.agent.a.schedule().push_back(stop);
            this->adding_stop = false;
            return;
        } 
        *this->selected_info_bottom = ui::Element().as_phantom().as_movable();
        *this->selected_info_right = ui::Element().as_phantom().as_movable();
        this->selected_type = SelectionType::None;
        this->adding_stop = false;
        if(building != nullptr) {
            std::span<const Conversion> conversions;
            if(building->complex.has_value()) {
                u64 ax = b_chunk_x * this->world->terrain.tiles_per_chunk() 
                    + building->x;
                u64 az = b_chunk_z * this->world->terrain.tiles_per_chunk() 
                    + building->z;
                const Complex& complex = this->world->complexes
                    .get(*building->complex);
                const Complex::Member& member = complex.member_at(ax, az);
                conversions = member.conversions;
                this->selected_type = SelectionType::Complex;
                this->selected.complex = *building->complex;
            }
            *this->selected_info_bottom = TerrainMap::display_building_info(
                building->type, conversions, *this->local
            );
            *this->selected_info_right = building->complex.has_value()
                ? TerrainMap::display_complex_info(
                    this->world->complexes.get(*building->complex), *this->local
                )
                : ui::Element().as_phantom().as_movable();
        }
        this->render_map();
    }

    void TerrainMap::update(const engine::Window& window, engine::Scene& scene) {
        this->local = &scene.get(this->local_ref);
        if(this->container == nullptr) { return; }
        if(this->container->hidden) { return; }
        this->update_view(window);
        this->update_click(window);
        if(this->selected_type == SelectionType::Complex) {
            const Complex& complex = this->world->complexes
                .get(this->selected.complex);
            *this->selected_info_right 
                = TerrainMap::display_complex_info(complex, *this->local);
        }
        if(this->selected_type == SelectionType::Agent) {
            *this->selected_info_right = this->display_agent_info(
                this->selected.agent.a, *this->selected.agent.d
            );
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
        this->output_tex.as_target().clear_color(
            Vec<4>(
                background_color.r, background_color.g, background_color.b, 
                background_color.a
            ) / 255.0
        );
        this->view_size_px.x() = (f64) this->output_tex.height() 
            * this->view_scale;
        this->view_size_px.y() = (f64) this->t_width / this->t_height 
            * this->view_size_px.x();
        this->view_pos_px = Vec<2>(this->output_tex.width(), this->output_tex.height()) / 2
            - this->view_size_px * this->view_pos;
        this->rendered_tex.blit(
            this->output_tex.as_target(), 
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
            .with_child(create_single_marker_info(
                &ui_icon::map_marker_train, 
                this->local->text("ui_train")
            ))
            .with_child(create_single_marker_info(
                &ui_icon::map_marker_boat, 
                this->local->text("ui_boat")
            ))
            .as_movable()
        );
    }

    void TerrainMap::add_marker(Vec<2> pos, ui::Element&& element) {
        Vec<2> pos_norm = pos 
            / this->world->terrain.units_per_tile()
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

    void TerrainMap::add_agent_stop_marker(AbstractAgent agent, size_t stop_i) {
        Vec<3> ag_tile_pos = agent.position() 
            / this->world->terrain.units_per_tile();
        const Complex& complex = this->world->complexes
            .get(agent.schedule()[stop_i].target);
        const auto& [mem_x, mem_z] = complex
            .closest_member_to((u64) ag_tile_pos.x(), (u64) ag_tile_pos.z());
        const Building* member = this->world->terrain
            .building_at((i64) mem_x, (i64) mem_z);
        assert(member != nullptr);
        const Building::TypeInfo& building = member->get_type_info();
        Vec<2> marker_pos = Vec<2>(
            mem_x + building.width / 2.0, mem_z + building.height / 2.0
        ) * this->world->terrain.units_per_tile();
        this->add_marker(marker_pos, ui::Element()
            .as_phantom()
            .with_size(0, 0, ui::size::unwrapped_text)
            .with_text(
                "#" + std::to_string(stop_i + 1), &ui_font::dark
            )
            .as_movable()
        );
    }

    void TerrainMap::add_agent_markers(
        AbstractAgent agent, const AgentDisplay& agent_display
    ) {
        bool is_selected = this->selected_type == SelectionType::Agent
            && this->selected.agent.a.data == agent.data;
        this->add_icon_marker(
            agent.position().swizzle<2>("xz"),
            agent.state() == AgentState::Lost
                ? &ui_icon::map_marker_agent_lost
                : is_selected
                    ? &ui_icon::map_marker_selected
                    : agent_display.marker,
            [this, agent, ag_d = &agent_display]() {
                this->selected_info_bottom->hidden = true;
                *this->selected_info_right 
                    = this->display_agent_info(agent, *ag_d);
                this->selected_type = SelectionType::Agent;
                this->selected.agent.a = agent;
                this->selected.agent.d = ag_d;
                this->render_map();
            }, 
            false
        );
        if(!is_selected) { return; }
        for(size_t stop_i = 0; stop_i < agent.schedule().size(); stop_i += 1) {
            this->add_agent_stop_marker(agent, stop_i);
        }
    }

    void TerrainMap::create_markers() {
        if(this->container == nullptr) { return; }
        if(this->container->hidden) { return; }
        this->container->children.clear();
        for(Carriage& carriage: this->world->carriages.agents) {
            this->add_agent_markers(
                carriage.as_abstract(), TerrainMap::carriage_display
            );
        }
        for(Train& train: this->world->trains.agents) {
            this->add_agent_markers(
                train.as_abstract(), TerrainMap::train_display
            );
        }
        for(Boat& boat: this->world->boats.agents) {
            this->add_agent_markers(
                boat.as_abstract(), TerrainMap::boat_display
            );
        }
        this->add_icon_marker(
            this->world->personal_horse.position().swizzle<2>("xz"), 
            &ui_icon::map_marker_personal_horse,
            [](){}, true
        );
        this->add_icon_marker(
            this->world->player.character.position.swizzle<2>("xz"), 
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
        const Item::TypeInfo& item = Item::types().at((size_t) type);
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
        const Building::TypeInfo& building = Building::types().at((size_t) type);
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
        static const f64 display_prec = 1000;
        static const f64 display_epsilon = 0.00001;
        std::unordered_map<Item::Type, f64> throughput 
            = complex.compute_throughput();
        ui::Element inputs = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Vertical)
            .as_movable();
        for(const auto& [item, freq]: throughput) {
            f64 d_freq = trunc(freq * display_prec) / display_prec;
            if(d_freq >= -display_epsilon) { continue; }
            inputs.children.push_back(TerrainMap::display_item_stack(
                item, std::format("{}", fabs(d_freq)), local
            ));
        }
        ui::Element outputs = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Vertical)
            .as_movable();
        for(const auto& [item, freq]: throughput) {
            f64 d_freq = trunc(freq * display_prec) / display_prec;
            if(d_freq <= display_epsilon) { continue; }
            outputs.children.push_back(TerrainMap::display_item_stack(
                item, std::format("{}", d_freq), local
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

    static const size_t max_column_items = 15;
    
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
            .with_list_dir(ui::Direction::Horizontal)
            .with_pos(0.5, 0.5, ui::position::window_fract)
            .as_movable();
        size_t column_count = (items.size() / max_column_items) + 1;
        for(size_t column_i = 0; column_i < column_count; column_i += 1) {
            ui::Element column = ui::Element()
                .with_size(0, 0, ui::size::units_with_children)
                .with_list_dir(ui::Direction::Vertical)
                .as_movable();
            size_t first_item_i = column_i * max_column_items;
            size_t end_item_i = std::min(
                first_item_i + max_column_items, items.size()
            );
            for(size_t item_i = first_item_i; item_i < end_item_i; item_i += 1) {
                Item::Type item = items[item_i];
                const Item::TypeInfo& item_info = Item::types().at((size_t) item);
                column.children.push_back(TerrainMap::create_selection_item(
                    item_info.icon, local.text(item_info.local_name), false,
                    [item, handler]() { (*handler)(item); }
                ));
            }
            container.children.push_back(std::move(column));
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

    ui::Element TerrainMap::display_agent_stop(
        AbstractAgent agent, size_t stop_i
    ) {
        AgentStop* stop = &agent.schedule()[stop_i];
        std::string local_action = "";
        switch(stop->action) {
            case AgentStop::Load: local_action = "ui_pick_up"; break;
            case AgentStop::Unload: local_action = "ui_drop_off"; break;
            case AgentStop::Maintain: local_action = "ui_maintain"; break;
        }
        std::string amount = "";
        std::string unit = "";
        switch(stop->unit) {
            case AgentStop::Fixed:
                amount = std::to_string(stop->amount.fixed);
                unit = "x";
                break;
            case AgentStop::Fraction:
                amount = std::to_string(
                    (u64) round(stop->amount.fract * 100.0)
                );
                unit = "%";
                break;
        }
        std::string_view local_item 
            = Item::types().at((size_t) stop->item).local_name;
        ui::Element info = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Horizontal)
            .with_child(make_ui_button("🗑")
                .with_click_handler([agent, stop_i]() {
                    agent.schedule().erase(
                        agent.schedule().begin() + stop_i
                    );
                    agent.reset_path();
                })
                .with_padding(2)
                .as_movable()
            )
            .with_child(make_ui_button("↑")
                .with_click_handler([agent, stop_i]() {
                    size_t swapped_with_i = stop_i == 0
                        ? agent.schedule().size() - 1
                        : stop_i - 1;
                    std::swap(
                        agent.schedule()[swapped_with_i],
                        agent.schedule()[stop_i]
                    );
                    agent.reset_path();
                })
                .with_padding(2)
                .as_movable()
            )
            .with_child(make_ui_button("↓")
                .with_click_handler([agent, stop_i]() {
                    size_t swapped_with_i = (stop_i + 1)
                        % agent.schedule().size();
                    std::swap(
                        agent.schedule()[swapped_with_i],
                        agent.schedule()[stop_i]
                    );
                    agent.reset_path();
                })
                .with_padding(2)
                .as_movable()
            )
            .with_child(ui::Element()
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(
                    "#" + std::to_string(stop_i + 1), &ui_font::dark
                )
                .with_padding(3)
                .as_movable()
            )
            .with_child(make_ui_button(this->local->text(local_action))
                .with_click_handler([stop]() {
                    switch(stop->action) {
                        case AgentStop::Load:
                            stop->action = AgentStop::Unload; break;
                        case AgentStop::Unload:
                            stop->action = AgentStop::Maintain; break;
                        case AgentStop::Maintain:
                            stop->action = AgentStop::Load; break;
                    }
                })
                .with_padding(2)
                .as_movable()
            )
            .with_child(make_ui_button("+")
                .with_click_handler([stop]() {
                    switch(stop->unit) {
                        case AgentStop::Fixed: 
                            stop->amount.fixed += 1;
                            break;
                        case AgentStop::Fraction:
                            i64 steps = (i64) round(stop->amount.fract / 0.05);
                            stop->amount.fract = (f32) (steps + 1) * 0.05;
                            stop->amount.fract = std::min(
                                std::max(stop->amount.fract, (f32) 0), (f32) 1
                            );
                            break;
                    }
                })
                .with_padding(2)
                .as_movable()
            )
            .with_child(make_ui_button("-")
                .with_click_handler([stop]() {
                    switch(stop->unit) {
                        case AgentStop::Fixed: 
                            if(stop->amount.fixed > 0) { 
                                stop->amount.fixed -= 1; 
                            }
                            break;
                        case AgentStop::Fraction:
                            i64 steps = (i64) round(stop->amount.fract / 0.05);
                            stop->amount.fract = (f32) (steps - 1) * 0.05;
                            stop->amount.fract = std::min(
                                std::max(stop->amount.fract, (f32) 0), (f32) 1
                            );
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
                .with_click_handler([stop]() {
                    switch(stop->unit) {
                        case AgentStop::Fixed:
                            stop->unit = AgentStop::Fraction; 
                            stop->amount.fract = 1.0;
                            break;
                        case AgentStop::Fraction:
                            stop->unit = AgentStop::Fixed; 
                            stop->amount.fixed = 0;
                            break;
                    }
                })
                .with_padding(2)
                .as_movable()
            )
            .with_child(make_ui_button(this->local->text(local_item))
                .with_click_handler([this, stop]() {
                    this->adding_stop = false;
                    std::vector<Item::Type> transferrable;
                    for(size_t item_i = 0; item_i < Item::types().size(); item_i += 1) {
                        if(!Item::types().at(item_i).storable) { continue; }
                        transferrable.push_back((Item::Type) item_i);
                    }
                    *this->selected_info_bottom 
                        = TerrainMap::display_item_selector(
                            transferrable, [this, stop](auto selected) {
                                stop->item = selected;
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

    ui::Element TerrainMap::display_agent_info(
        AbstractAgent agent, const AgentDisplay& agent_display
    ) {
        std::string status;
        switch(agent.state()) {
            case AgentState::Idle:
                status = "ui_status_no_instructions"; break;
            case AgentState::Travelling: 
                status = "ui_status_travelling"; break;
            case AgentState::Loading: 
                status = "ui_status_transferring"; break;
            case AgentState::Lost: 
                status = "ui_status_lost"; break;
        }
        std::string status_text = this->local->pattern(
            status, { std::to_string(agent.stop_i() + 1) }
        );
        ui::Element storage = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Vertical)
            .as_movable();
        for(const auto& [item, count]: agent.items()) {
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
        ui::Element schedule_info = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Vertical)
            .as_movable();
        for(u64 stop_i = 0; stop_i < agent.schedule().size(); stop_i += 1) {
            schedule_info.children
                .push_back(this->display_agent_stop(agent, stop_i));
        }
        if(schedule_info.children.size() == 0) {
            schedule_info.children.push_back(ui::Element()
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
            .as_movable();
        info.children.push_back(ui::Element()
            .with_size(0, 0, ui::size::unwrapped_text)
            .with_text(
                this->local->text(agent_display.local_title), &ui_font::dark
            )
            .with_padding(1)
            .as_movable()
        );
        info.children.push_back(ui::Element()
            .with_size(0, 0, ui::size::unwrapped_text)
            .with_text(status_text, &ui_font::dark)
            .with_padding(3)
            .as_movable()
        );
        ui::Element buttons = ui::Element()
            .as_phantom()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Vertical)
            .as_movable();
            buttons.children.push_back(
            make_ui_button(this->local->text(agent_display.local_remove))
                .with_click_handler([this, agent, ag_d = &agent_display]() {
                    ag_d->remove_impl(agent, *this->world);
                    this->selected_info_right->hidden = true;
                    this->selected_info_bottom->hidden = true;
                    this->selected_type = SelectionType::None;
                    this->adding_stop = false;
                })
                .with_padding(2)
                .as_movable()
        );
        for(const auto& button: agent_display.buttons) {
            ui::Element button_elem 
                = make_ui_button(this->local->text(button.local_text))
                .with_click_handler([h = button.handler, agent, this]() {
                    h(agent, *this->world, this->toasts);
                })
                .with_padding(2)
                .as_movable();
            buttons.children.push_back(std::move(button_elem));
        }
        info.children.push_back(buttons.with_padding(2).as_movable());
        info.children.push_back(ui::Element()
            .with_size(0, 0, ui::size::unwrapped_text)
            .with_text(this->local->text("ui_schedule"), &ui_font::dark)
            .with_padding(1)
            .as_movable()
        );
        info.children.push_back(schedule_info.with_padding(3.0).as_movable());
        info.children.push_back(std::move(add_stop));
        info.children.push_back(ui::Element()
            .with_size(0, 0, ui::size::unwrapped_text)
            .with_text(this->local->text("ui_on_board"), &ui_font::dark)
            .with_padding(1)
            .as_movable()
        );
        info.children.push_back(storage.with_padding(3.0).as_movable());
        return info;
    }

}