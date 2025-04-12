
#include "common.hpp"

namespace houseofatmos::world {

    namespace ui = houseofatmos::engine::ui;


    TerraformMode::TerraformMode(ActionContext ctx): ActionMode(ctx) {
        this->has_selection = false;
    }

    void TerraformMode::init_ui() {
        this->ui.with_element(ui::Element()
            .with_pos(
                ui::unit * 50, 
                ui::height::window - ui::unit * 15 - ui::vert::height
            )
            .with_background(&ui_background::note)
            .with_list_dir(ui::Direction::Horizontal)
            .as_movable()
        );
        this->mode_selection = &this->ui.root.children.back();
        this->ui.with_element(ui::Element()
            .as_phantom()
            .with_pos(ui::null, ui::null)
            .with_size(ui::width::window, ui::height::window)
            .as_movable()
        );
        this->vertex_markers = &this->ui.root.children.back();
        this->set_mode(Mode::Flatten);
    }

    void TerraformMode::set_mode(Mode mode) {
        this->mode = mode;
        this->mode_selection->children.clear();
        for(size_t mode_i = 0; mode_i < (size_t) Mode::TotalCount; mode_i += 1) {
            bool is_selected = mode_i == (size_t) mode;
            this->mode_selection->children.push_back(ui::Element()
                .as_phantom()
                .with_size(ui::unit * 16, ui::unit * 16)
                .with_background(TerraformMode::mode_icons[mode_i])
                .with_child(ui::Element()
                    .with_size(ui::width::parent, ui::height::parent)
                    .with_click_handler([this, mode_i]() {
                        if(mode_i == (u64) this->mode) { return; }
                        this->set_mode((Mode) mode_i);
                    })
                    .with_background(
                        is_selected? &ui_background::border_selected
                            : &ui_background::border,
                        is_selected? &ui_background::border_selected
                            : &ui_background::border_hovering
                    )
                    .as_movable()
                )
                .with_padding(2)
                .as_movable()
            );
        }
    }

    static i64 terrain_mod_amount(
        i16 elev, TerraformMode::Mode mode, i16 start_elev
    ) {
        switch(mode) {
            case TerraformMode::Flatten:
                return (i64) start_elev - (i64) elev;
            case TerraformMode::Raise:
                return 1;
            case TerraformMode::Lower:
                return -1;
            default:
                return 0;
        }
    }

    static i64 terrain_mod_amount(
        const Terrain& terrain, u64 x, u64 z,
        TerraformMode::Mode mode, i16 start_elev
    ) {
        switch(mode) {
            case TerraformMode::Flatten: {
                i16 elev = terrain.elevation_at(x, z);
                return (i64) start_elev - (i64) elev;
            }
            case TerraformMode::Raise:
                return 1;
            case TerraformMode::Lower:
                return -1;
            default:
                return 0;
        }
    }

    static u64 area_abs_terrain_mod_amount(
        const Terrain& terrain, TerraformMode::Mode mode,
        u64 min_x, u64 min_z, u64 max_x, u64 max_z, i16 start_elev
    ) {
        u64 sum = 0;
        for(u64 x = min_x; x <= max_x; x += 1) {
            for(u64 z = min_z; z <= max_z; z += 1) {
                sum += (u64) std::abs(terrain_mod_amount(
                    terrain, x, z, mode, start_elev
                ));
            }
        }
        return sum;
    }

    static void apply_terrain_modification(
        Terrain& terrain, TerraformMode::Mode mode,
        u64 min_x, u64 min_z, u64 max_x, u64 max_z, i16 start_elev
    ) {
        for(u64 x = min_x; x <= max_x; x += 1) {
            for(u64 z = min_z; z <= max_z; z += 1) {
                auto& e = terrain.elevation_at(x, z);
                i64 mod = terrain_mod_amount(terrain, x, z, mode, start_elev);
                e = (i16) ((i64) e + mod);
            }
        }
        for(u64 x = min_x > 0? min_x - 1: 0; x <= max_x; x += 1) {
            for(u64 z = min_z > 0? min_z - 1: 0; z <= max_z; z += 1) {
                u64 rx = std::min(x + 1, terrain.width_in_tiles());
                u64 bz = std::min(z + 1, terrain.height_in_tiles());
                bool allows_path = terrain.elevation_at(x, z) >= 0
                        && terrain.elevation_at(rx, z) >= 0
                        && terrain.elevation_at(x, bz) >= 0
                        && terrain.elevation_at(rx, bz) >= 0;
                if(!allows_path) {
                    u64 tpc = terrain.tiles_per_chunk();
                    terrain.chunk_at(x / tpc, z / tpc)
                        .set_path_at(x % tpc, z % tpc, false);
                }
            }
        }
    }

    static const u64 terrain_mod_cost_per_unit = 50;

    void TerraformMode::update(
        const engine::Window& window, engine::Scene& scene, 
        const Renderer& renderer
    ) {
        (void) scene;
        (void) renderer;
        this->speaker.update();
        // figure out the selection area
        auto [tile_x, tile_z] = this->world->terrain.find_selected_terrain_tile(
            window.cursor_pos_ndc(), renderer, Vec<3>(0, 0, 0)
        );
        if(!this->has_selection) {
            this->selection = (Selection) { tile_x, tile_z, tile_x, tile_z };
        }
        this->has_selection |= (
            window.is_down(engine::Button::Left) && !this->ui.is_hovered_over()
        );
        if(window.is_down(engine::Button::Left)) {
            this->has_selection = true;
        }
        if(this->has_selection) {
            this->selection.end_x = tile_x;
            this->selection.end_z = tile_z;
        }
        // get selection bounds
        u64 min_x = std::min(this->selection.start_x, this->selection.end_x);
        u64 min_z = std::min(this->selection.start_z, this->selection.end_z);
        u64 max_x = std::max(this->selection.start_x, this->selection.end_x);
        u64 max_z = std::max(this->selection.start_z, this->selection.end_z);
        // do terrain manipulation operation if needed
        bool attempt_operation = this->has_selection
            && !window.is_down(engine::Button::Left);
        if(attempt_operation && !this->ui.is_hovered_over()) {
            // compute cost
            i16 start_elev = this->world->terrain
                .elevation_at(this->selection.start_x, this->selection.start_z);
            u64 cost = area_abs_terrain_mod_amount(
                this->world->terrain, 
                this->mode, min_x, min_z, max_x, max_z, start_elev
            ) * terrain_mod_cost_per_unit;
            bool is_valid = this->world->terrain.vert_area_elev_mutable(
                min_x, min_z, max_x, max_z,
                [this, start_elev](i16 elev) { 
                    return terrain_mod_amount(elev, this->mode, start_elev) != 0; 
                }
            );
            bool do_operation = is_valid 
                && this->permitted
                && this->world->balance.pay_coins(cost, this->toasts);
            if(do_operation) {
                apply_terrain_modification(
                    this->world->terrain, 
                    this->mode, min_x, min_z, max_x, max_z, start_elev
                );
                this->world->terrain.adjust_area_foliage(
                    min_x - 1, min_z - 1, max_x + 1, max_z + 1
                );
                this->world->boats.find_paths(&this->toasts);
                this->speaker.position = tile_bounded_position(
                    min_x, min_z, max_x, max_z,
                    this->world->player.character.position,
                    this->world->terrain
                );
                this->speaker.play(scene.get(sound::terrain_mod));
            }
            if(!is_valid) {
                this->toasts.add_error("toast_terrain_occupied", {});
            }
        }
        this->has_selection &= !attempt_operation;
        // display the manipulated vertices
        this->vertex_markers->children.clear();
        for(u64 x = min_x; x <= max_x; x += 1) {
            for(u64 z = min_z; z <= max_z; z += 1) {
                f64 elev = (f64) this->world->terrain.elevation_at(x, z);
                Vec<3> world = Vec<3>(
                    x * this->world->terrain.units_per_tile(), elev,
                    z * this->world->terrain.units_per_tile()
                );
                Vec<2> ndc = renderer.world_to_ndc(world);
                this->vertex_markers->children.push_back(ui::Element()
                    .as_phantom()
                    .with_pos(
                        ui::horiz::window_ndc(ndc.x()) - ui::unit * 4, 
                        ui::vert::window_ndc(ndc.y()) - ui::unit * 4
                    )
                    .with_size(ui::unit * 8, ui::unit * 8)
                    .with_background(&ui_icon::terrain_vertex)
                    .as_movable()
                );
            }
        }
    }    

}