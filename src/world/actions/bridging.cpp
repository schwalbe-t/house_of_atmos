
#include "common.hpp"
#include "../../ui_util.hpp"

namespace houseofatmos::world {

    namespace ui = houseofatmos::engine::ui;


    static const std::vector<std::optional<research::Research::Reward>> 
            required_bridge_rewards = {
        std::nullopt, // Bridge::Type::Wooden
        std::nullopt, // Bridge::Type::Stone
        research::Research::Reward::SteelBridges
    };

    static ui::Element create_bridge_selector(
        ui::Manager* ui, ui::Element* dest,
        Bridge::Type* s_type,
        const engine::Localization* local, const research::Research* research
    ) {
        ui::Element selector 
            = ui_util::create_selection_container(
                local->text("ui_bridge_selection")
            )
            .with_pos(0.95, 0.5, ui::position::window_fract)
            .as_movable();
        for(size_t type_id = 0; type_id < Bridge::types().size(); type_id += 1) {
            auto req_reward = required_bridge_rewards[type_id];
            bool unlocked = !req_reward.has_value()
                || research->is_unlocked(*req_reward);
            if(!unlocked) { continue; }
            const Bridge::TypeInfo& type = Bridge::types().at(type_id);
            selector.children.push_back(ui_util::create_selection_item(
                type.icon, local->text(type.local_name), 
                (size_t) *s_type == type_id,
                [ui, dest, s_type, local, research, type_id]() {
                    *s_type = (Bridge::Type) type_id;
                    *dest = create_bridge_selector(
                        ui, dest, s_type, local, research
                    );
                }
            ));
        }
        return selector;
    }

    BridgingMode::BridgingMode(ActionContext ctx): ActionMode(ctx) {
        this->selected_type = std::make_unique<Bridge::Type>(Bridge::Wooden);
    }

    void BridgingMode::init_ui() {
        this->ui.root.children.push_back(ui::Element());
        ui::Element* selector = &this->ui.root.children.back();
        *selector = create_bridge_selector(
            &this->ui, selector, this->selected_type.get(), 
            &local, &this->world->research
        );
    }

    Bridge BridgingMode::get_planned() const {
        u64 start_x = this->selection.start_x;
        u64 start_z = this->selection.start_z;
        u64 end_x = this->selection.end_x;
        u64 end_z = this->selection.end_z;
        bool x_diff_larger = end_x - start_x > end_z - start_z;
        if(x_diff_larger) { end_z = start_z; }
        else { end_x = start_x; }
        i16 height = INT16_MIN;
        height = std::max(height, this->world->terrain.elevation_at(start_x, start_z));
        height = std::max(height, this->world->terrain.elevation_at(start_x + 1, start_z));
        height = std::max(height, this->world->terrain.elevation_at(start_x, start_z + 1));
        height = std::max(height, this->world->terrain.elevation_at(start_x + 1, start_z + 1));
        return (Bridge) { 
            *this->selected_type,
            std::min(start_x, end_x), std::min(start_z, end_z), 
            std::max(start_x, end_x), std::max(start_z, end_z),
            height
        };
    }

    i64 BridgingMode::get_reduced_planned_height(
        i64 start, i64 (*reduce)(i64 acc, i64 height)
    ) const {
        i64 result = start;
        bool is_horizontal = this->planned.start_z == this->planned.end_z;
        u64 x = this->planned.start_x + (is_horizontal? 1 : 0);
        u64 z = this->planned.start_z + (is_horizontal? 0 : 1);
        u64 end_x = this->planned.end_x 
            - (this->planned.end_x == 0? 0 : is_horizontal? 1 : 0);
        u64 end_z = this->planned.end_z 
            - (this->planned.end_x == 0? 0 : is_horizontal? 0 : 1);
        for(;;) {
            i64 e_tl = this->world->terrain.elevation_at(x, z);
            result = reduce(result, this->planned.floor_y - e_tl);
            i64 e_tr = this->world->terrain.elevation_at(x + 1, z);
            result = reduce(result, this->planned.floor_y - e_tr);
            i64 e_bl = this->world->terrain.elevation_at(x, z + 1);
            result = reduce(result, this->planned.floor_y - e_bl);
            i64 e_br = this->world->terrain.elevation_at(x + 1, z + 1);
            result = reduce(result, this->planned.floor_y - e_br);
            if(x < end_x) { x += 1; }
            else if(z < end_z) { z += 1; }
            else { break; }
        }
        return result;
    };

    bool BridgingMode::planned_ends_match() const {
        u64 left = this->planned.start_x;
        u64 right = this->planned.end_x + 1;
        u64 top = this->planned.start_z;
        u64 bottom = this->planned.end_z + 1;
        i16 tl = this->world->terrain.elevation_at(left, top);
        i16 tr = this->world->terrain.elevation_at(right, top);
        i16 bl = this->world->terrain.elevation_at(left, bottom);
        i16 br = this->world->terrain.elevation_at(right, bottom);
        return tl == tr && tr == bl && bl == br;
    }

    bool BridgingMode::planned_is_occupied() const {
        u64 x = this->planned.start_x;
        u64 z = this->planned.start_z;
        for(;;) {
            bool occupied = this->world->terrain.bridge_at((i64) x, (i64) z) != nullptr
                || this->world->terrain.building_at((i64) x, (i64) z) != nullptr;
            if(occupied) { return true; }
            if(x < this->planned.end_x) { x += 1; }
            else if(z < this->planned.end_z) { z += 1; }
            else { break; }
        }
        return false;
    }

    void BridgingMode::update(
        const engine::Window& window, engine::Scene& scene, 
        const Renderer& renderer
    ) {
        (void) scene;
        this->speaker.update();
        // update the selection area
        auto [tile_x, tile_z] = this->world->terrain.find_selected_terrain_tile(
            window.cursor_pos_ndc(), renderer, Vec<3>(0, 0, 0)
        );
        if(!this->has_selection) {
            this->selection = (Selection) { tile_x, tile_z, tile_x, tile_z };
        }
        this->has_selection |= (
            window.is_down(engine::Button::Left) && !this->ui.is_hovered_over()
        );
        if(this->has_selection) {
            this->selection.end_x = tile_x;
            this->selection.end_z = tile_z;
        }
        // determine information about the bridge's actual placement
        this->planned = this->get_planned();
        i64 min_height = this->get_reduced_planned_height(
            INT64_MAX, [](auto acc, auto h) { return std::min(acc, h); }
        );
        i64 max_height = this->get_reduced_planned_height(
            -INT64_MAX, [](auto acc, auto h) { return std::max(acc, h); }
        );
        // determine if the placement is valid
        const Bridge::TypeInfo& selected_type 
            = Bridge::types().at((size_t) *this->selected_type);
        bool too_short = this->planned.length() <= 1;
        bool too_low = min_height < selected_type.min_height;
        bool too_high = max_height > selected_type.max_height;
        bool occupied = this->planned_is_occupied();
        bool ends_match = this->planned_ends_match();
        bool obstructed = occupied || too_low;
        this->placement_valid = !too_short && !too_high
            && !obstructed && ends_match;
        // do placement
        u64 cost = this->planned.length() * selected_type.cost_per_tile;
        bool attempted_placement = this->has_selection 
            && !window.is_down(engine::Button::Left);
        if(attempted_placement && !this->ui.is_hovered_over()) {
            if(too_short) { 
                this->toasts.add_error("toast_bridge_too_short", {});
            }
            if(too_high) {
                this->toasts.add_error("toast_bridge_too_high", {});
            }
            if(!ends_match) {
                this->toasts.add_error("toast_bridge_ends_dont_match", {});
            }
            bool doing_placement = this->placement_valid 
                && this->permitted
                && this->world->balance.pay_coins(cost, this->toasts);
            if(doing_placement) {
                this->world->terrain.bridges.push_back(this->planned);
                this->world->carriages.find_paths(&this->toasts);
                this->speaker.position = tile_bounded_position(
                    this->planned.start_x, this->planned.start_z, 
                    this->planned.end_x, this->planned.end_z,
                    this->world->player.character.position,
                    this->world->terrain
                );
                this->speaker.play(scene.get(sound::build));
            }
        }
        this->has_selection &= !attempted_placement;
    }

    void BridgingMode::render(
        const engine::Window& window, engine::Scene& scene, 
        Renderer& renderer
    ) {
        (void) window;
        this->planned = this->get_planned();
        const engine::Texture& wireframe_texture = !this->has_selection
            ? scene.get(ActionMode::wireframe_info_texture)
            : this->placement_valid
                ? scene.get(ActionMode::wireframe_valid_texture)
                : scene.get(ActionMode::wireframe_error_texture);
        engine::Model& model = scene.get(
            Bridge::types().at((size_t) *this->selected_type).model
        );
        renderer.render(
            model, 
            this->planned.get_instances(this->world->terrain.units_per_tile()),
            nullptr, 0.0,
            engine::FaceCulling::Enabled,
            engine::Rendering::Wireframe, 
            engine::DepthTesting::Enabled,
            &wireframe_texture
        );
    }
    
}