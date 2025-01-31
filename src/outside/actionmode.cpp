
#include "actionmode.hpp"
#include "terrainmap.hpp"
#include <iostream>

namespace houseofatmos::outside {

    namespace ui = houseofatmos::engine::ui;



    static const u64 carriage_buy_cost = 1000;
    static const u64 carr_spawn_d = 1;

    static void summon_carriage(
        const Terrain& terrain, u64 stable_x, u64 stable_z,
        Balance& balance, Toasts& toasts, CarriageManager& carriages
    ) {
        Vec<3> pos;
        bool found_pos = false;
        const Building::TypeInfo& stable
            = Building::types[(size_t) Building::Stable];
        i64 start_x = stable_x - carr_spawn_d;
        i64 start_z = stable_z - carr_spawn_d;
        i64 end_x = stable_x + stable.width + carr_spawn_d;
        i64 end_z = stable_z + stable.height + carr_spawn_d;
        for(i64 x = start_x; x < end_x; x += 1) {
            for(i64 z = start_z; z < end_z; z += 1) {
                if(x < 0 || (u64) x >= terrain.width_in_tiles()) { continue; }
                if(z < 0 || (u64) z >= terrain.height_in_tiles()) { continue; }
                u64 chunk_x = (u64) x / terrain.tiles_per_chunk();
                u64 chunk_z = (u64) z / terrain.tiles_per_chunk();
                const Terrain::ChunkData& chunk = terrain
                    .chunk_at(chunk_x, chunk_z);
                u64 rel_x = (u64) x % terrain.tiles_per_chunk();
                u64 rel_z = (u64) z % terrain.tiles_per_chunk();
                bool is_obstacle = !chunk.path_at(rel_x, rel_z)
                    || terrain.building_at(x, z) != nullptr;
                if(is_obstacle) { continue; }
                pos = Vec<3>(x + 0.5, 0, z + 0.5) * terrain.units_per_tile();
                found_pos = true;
                break;
            }
        }
        if(!found_pos) {
            toasts.add_error("toast_no_valid_carriage_location", {});
            return;
        }
        if(!balance.pay_coins(carriage_buy_cost, toasts)) { return; }
        carriages.carriages.push_back((Carriage) { Carriage::Round, pos });
    }

    void DefaultMode::update(
        const engine::Window& window, engine::Scene& scene, 
        const Renderer& renderer
    ) {
        (void) scene;
        (void) renderer;
        this->local = &this->toasts.localization();
        auto [s_tile_x, s_tile_z] = this->terrain.find_selected_terrain_tile(
            window.cursor_pos_ndc(), renderer, Vec<3>(0.5, 0, 0.5)
        );
        u64 s_chunk_x, s_chunk_z;
        Building* s_building = this->terrain.building_at(
            (i64) s_tile_x, (i64) s_tile_z, &s_chunk_x, &s_chunk_z
        );
        bool clicked_world = window.was_pressed(engine::Button::Left)
            && !this->ui.was_clicked();
        bool clicked_stable = s_building != nullptr
            && s_building->type == Building::Stable
            && clicked_world;
        if(clicked_stable) {
            u64 asx = s_chunk_x * this->terrain.tiles_per_chunk() + s_building->x;
            u64 asz = s_chunk_z * this->terrain.tiles_per_chunk() + s_building->z;
            *this->button = ui::Element()
                .as_phantom()
                .with_pos(0.5, 0.95, ui::position::window_fract)
                .with_size(0, 0, ui::size::unwrapped_text)
                .with_text(
                    this->local->text("ui_create_carriage"), &ui_font::bright
                )
                .with_padding(3)
                .with_background(
                    &ui_background::button, &ui_background::button_select
                )
                .with_click_handler([this, asx, asz]() {
                    summon_carriage(
                        this->terrain, asx, asz, 
                        this->balance, this->toasts, this->carriages
                    );
                })
                .as_movable();
        } else if(clicked_world) {
            this->button->hidden = true;
        }
    }



    TerraformMode::TerraformMode(
        Terrain& terrain, ComplexBank& complexes, 
        CarriageManager& carriages, Player& player, Balance& balance,
        ui::Manager& ui, Toasts& toasts
    ): ActionMode(terrain, complexes, carriages, player, balance, ui, toasts) {
        this->has_selection = false;
        this->mode = std::make_unique<Mode>(Mode::Flatten);
        this->mode_buttons = std::make_unique<
            std::array<ui::Element*, (size_t) Mode::TotalCount>
        >();
        ui::Element mode_list = ui::Element()
            .with_pos(0.05, 0.80, ui::position::window_fract)
            .with_size(0, 0, ui::size::units_with_children)
            .with_background(&ui_background::note)
            .with_list_dir(ui::Direction::Horizontal)
            .as_movable();
        for(size_t mode_i = 0; mode_i < (size_t) Mode::TotalCount; mode_i += 1) {
            Mode* mode_ptr = this->mode.get();
            auto buttons = this->mode_buttons.get();
            mode_list.children.push_back(ui::Element()
                .with_size(16, 16, ui::size::units)
                .with_background(TerraformMode::mode_icons[mode_i])
                .with_click_handler([mode_ptr, mode_i, buttons]() {
                    if((size_t) (*mode_ptr) == mode_i) { return; }
                    *mode_ptr = (Mode) mode_i;
                    for(size_t i = 0; i < (size_t) Mode::TotalCount; i += 1) {
                        buttons->at(i)->background = i == mode_i
                            ? &ui_background::border_selected
                            : &ui_background::border;
                        buttons->at(i)->background_hover = i == mode_i
                            ? &ui_background::border_selected
                            : &ui_background::border_hovering;
                    }
                })
                .with_padding(0)
                .with_handle(&this->mode_buttons->at(mode_i))
                .with_background(
                    *mode_ptr == (Mode) mode_i
                        ? &ui_background::border_selected
                        : &ui_background::border,
                    *mode_ptr == (Mode) mode_i
                        ? &ui_background::border_selected
                        : &ui_background::border_hovering
                )
                .with_padding(2)
                .as_movable()
            );
        }
        ui.root.children.push_back(std::move(mode_list));
        ui.root.children.push_back(ui::Element()
            .as_phantom()
            .with_pos(0.0, 0.0, ui::position::window_tl_units)
            .with_size(1.0, 1.0, ui::size::window_fract)
            .with_handle(&this->vertex_markers)
            .as_movable()
        );
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

    static bool terrain_modification_is_valid(
        const Terrain& terrain,
        u64 min_x, u64 min_z, u64 max_x, u64 max_z,
        TerraformMode::Mode mode, i16 s_elev
    ) {
        u64 start_x = min_x >= 1? min_x - 1 : 0;
        u64 start_z = min_z >= 1? min_z - 1 : 0;
        u64 end_x = std::min(max_x, terrain.width_in_tiles() - 1);
        u64 end_z = std::min(max_z, terrain.height_in_tiles() - 1);
        for(u64 x = start_x; x <= end_x; x += 1) {
            for(u64 z = start_z; z <= end_z; z += 1) {
                bool has_building = (bool) terrain.building_at((i64) x, (i64) z);
                u64 lx = std::max(x, start_x + 1); // left x
                u64 rx = std::min(lx + 1, end_x); // right x
                u64 tz = std::max(z, start_z + 1); // top z
                u64 bz = std::min(tz + 1, end_z); // bottom z
                bool is_modified 
                    = terrain_mod_amount(terrain, lx, tz, mode, s_elev) != 0
                    || terrain_mod_amount(terrain, lx, bz, mode, s_elev) != 0
                    || terrain_mod_amount(terrain, rx, tz, mode, s_elev) != 0
                    || terrain_mod_amount(terrain, rx, bz, mode, s_elev) != 0;
                if(has_building && is_modified) { return false; }
            }
        }
        return true;
    }

    static const u64 terrain_mod_cost_per_vertex = 50;

    static u64 terrain_modification_cost(
        const Terrain& terrain, TerraformMode::Mode mode,
        u64 min_x, u64 min_z, u64 max_x, u64 max_z, i16 start_elev
    ) {
        u64 cost = 0;
        for(u64 x = min_x; x <= max_x; x += 1) {
            for(u64 z = min_z; z <= max_z; z += 1) {
                cost += (u64) std::abs(terrain_mod_amount(
                    terrain, x, z, mode, start_elev
                ));
            }
        }
        return cost * terrain_mod_cost_per_vertex;
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

    static void clear_area_foliage(
        Terrain& terrain, i64 start_x, i64 start_z, i64 end_x, i64 end_z
    ) {
        for(i64 x = start_x; x < end_x; x += 1) {
            for(i64 z = start_z; z < end_z; z += 1) {
                terrain.remove_foliage_at(x, z);
            }
        }
    }

    void TerraformMode::update(
        const engine::Window& window, engine::Scene& scene, 
        const Renderer& renderer
    ) {
        (void) scene;
        (void) renderer;
        // figure out the selection area
        auto [tile_x, tile_z] = this->terrain.find_selected_terrain_tile(
            window.cursor_pos_ndc(), renderer, Vec<3>(0, 0, 0)
        );
        if(!this->has_selection) {
            this->selection = (Selection) { tile_x, tile_z, tile_x, tile_z };
        }
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
            i16 start_elev = this->terrain
                .elevation_at(this->selection.start_x, this->selection.start_z);
            u64 cost = terrain_modification_cost(
                this->terrain, 
                *this->mode, min_x, min_z, max_x, max_z, start_elev
            );
            bool is_valid = terrain_modification_is_valid(
                this->terrain, min_x, min_z, max_x, max_z, 
                *this->mode, start_elev
            );
            if(is_valid && this->balance.pay_coins(cost, this->toasts)) {
                apply_terrain_modification(
                    this->terrain, 
                    *this->mode, min_x, min_z, max_x, max_z, start_elev
                );
                clear_area_foliage(
                    this->terrain, min_x - 1, min_z - 1, max_x + 1, max_z + 1
                );
            }
            if(!is_valid) {
                this->toasts.add_error("toast_terrain_occupied", {});
            }
        }
        if(attempt_operation) {
            this->has_selection = false;
        }
        // display the manipulated vertices
        this->vertex_markers->children.clear();
        for(u64 x = min_x; x <= max_x; x += 1) {
            for(u64 z = min_z; z <= max_z; z += 1) {
                f64 elev = (f64) this->terrain.elevation_at(x, z);
                Vec<3> world = Vec<3>(
                    x * this->terrain.units_per_tile(), elev,
                    z * this->terrain.units_per_tile()
                );
                Vec<2> ndc = renderer.world_to_ndc(world);
                this->vertex_markers->children.push_back(ui::Element()
                    .as_phantom()
                    .with_pos(ndc.x(), ndc.y(), ui::position::window_ndc)
                    .with_child(ui::Element()
                        .as_phantom()
                        .with_pos(-4, -4, ui::position::parent_offset_units)
                        .with_size(8, 8, ui::size::units)
                        .with_background(&ui_icon::terrain_vertex)
                        .as_movable()
                    )
                    .as_movable()
                );
            }
        }
    }



    using BuildingVariant = std::vector<Conversion>;
    using BuildingGroup = std::pair<Building::Type, std::vector<BuildingVariant>>;
    static const std::vector<BuildingGroup> buildable = {
        (BuildingGroup) { Building::House, {} },
        (BuildingGroup) { Building::Farmland, {
            { (Conversion) { 
                {}, 
                { { 10, Item::Wheat } }, 
                10.0 
            } },
            { (Conversion) { 
                {}, 
                { { 10, Item::Barley } }, 
                10.0 
            } }
        } },
        (BuildingGroup) { Building::Mineshaft, {
            { (Conversion) { 
                {}, 
                { { 1, Item::Hematite } }, 
                1.0 
            } },
            { (Conversion) { 
                {}, 
                { { 1, Item::Coal } }, 
                1.0 
            } }
        } },
        (BuildingGroup) { Building::Windmill, {
            { (Conversion) { 
                { { 4, Item::Barley } }, 
                { { 1, Item::Malt } }, 
                1.0 
            } },
            { (Conversion) { 
                { { 4, Item::Wheat } }, 
                { { 1, Item::Flour } }, 
                1.0 
            } }
        } },
        (BuildingGroup) { Building::Factory, {
            { (Conversion) { 
                { { 1, Item::Malt } }, 
                { { 4, Item::Beer } }, 
                4.0 
            } },
            { (Conversion) { 
                { { 1, Item::Flour } }, 
                { { 2, Item::Bread } }, 
                2.0 
            } },
            { (Conversion) { 
                { { 2, Item::Hematite }, { 1, Item::Coal } }, 
                { { 2, Item::Steel } }, 
                5.0 
            } },
            { (Conversion) { 
                { { 3, Item::Steel } }, 
                { { 1, Item::Armor } }, 
                10.0 
            } },
            { (Conversion) { 
                { { 2, Item::Steel } }, 
                { { 1, Item::Tools } }, 
                5.0 
            } }
        } },
        (BuildingGroup) { Building::Stable, {} }
    };

    static ui::Element create_building_selector(
        ui::Manager* ui, ui::Element* dest, ui::Element* selected,
        Building::Type* s_type, std::vector<Conversion>* s_conv,
        const engine::Localization* local
    );

    static ui::Element create_variant_selector(
        ui::Manager* ui, ui::Element* dest, ui::Element* selected,
        const BuildingGroup& group,
        Building::Type* s_type, std::vector<Conversion>* s_conv,
        const engine::Localization* local
    ) {
        ui::Element selector 
            = TerrainMap::create_selection_container(
                local->text("ui_product_selection")
            )
            .with_pos(0.95, 0.5, ui::position::window_fract)
            .as_movable();
        for(const BuildingVariant& variant: group.second) {
            if(variant.size() == 0) { continue; }
            if(variant.at(0).outputs.size() == 0) { continue; }
            const Item::TypeInfo& result = Item::items
                .at((size_t) variant.at(0).outputs.at(0).item);
            std::vector<Conversion> conversions = std::vector(variant);
            selector.children.push_back(TerrainMap::create_selection_item(
                result.icon, local->text(result.local_name), false,
                [
                    ui, dest, selected, s_type, s_conv, local, 
                    type = group.first, conversions
                ]() {
                    *s_type = type;
                    *s_conv = std::move(conversions);
                    *dest = create_building_selector(
                        ui, dest, selected, s_type, s_conv, local
                    );
                    *selected = TerrainMap::display_building_info(
                        *s_type, *s_conv, *local
                    );
                }
            ));
        }
        return selector;
    }   

    static ui::Element create_building_selector(
        ui::Manager* ui, ui::Element* dest, ui::Element* selected,
        Building::Type* s_type, std::vector<Conversion>* s_conv,
        const engine::Localization* local
    ) {
        ui::Element selector 
            = TerrainMap::create_selection_container(
                local->text("ui_building_selection")
            )
            .with_pos(0.95, 0.5, ui::position::window_fract)
            .as_movable();
        for(const BuildingGroup& group: buildable) {
            const BuildingGroup* group_ptr = &group;
            const Building::TypeInfo& type = Building::types
                .at((size_t) group.first);
            selector.children.push_back(TerrainMap::create_selection_item(
                type.icon, local->text(type.local_name), *s_type == group.first,
                [ui, dest, selected, s_type, s_conv, local, group_ptr]() {
                    if(group_ptr->second.size() == 0) {
                        *s_type = group_ptr->first;
                        s_conv->clear();
                        *dest = create_building_selector(
                            ui, dest, selected, s_type, s_conv, local
                        );
                        *selected = TerrainMap::display_building_info(
                            *s_type, *s_conv, *local
                        );
                        return;
                    }
                    *dest = create_variant_selector(
                        ui, dest, selected, *group_ptr, s_type, s_conv, local
                    );
                }
            ));
        }
        return selector;
    }

    ConstructionMode::ConstructionMode(
        Terrain& terrain, ComplexBank& complexes, 
        CarriageManager& carriages, Player& player, Balance& balance,
        ui::Manager& ui, Toasts& toasts
    ): ActionMode(terrain, complexes, carriages, player, balance, ui, toasts) {
        this->selected_x = 0;
        this->selected_z = 0;
        this->selected_type = std::make_unique<Building::Type>(Building::House);
        this->selected_conversion = std::make_unique<std::vector<Conversion>>();
        this->placement_valid = false;
        this->ui.root.children.push_back(ui::Element().as_phantom().as_movable());
        ui::Element* selector = &this->ui.root.children.back();
        this->ui.root.children.push_back(ui::Element().as_phantom().as_movable());
        ui::Element* selected = &this->ui.root.children.back();
        *selector = create_building_selector(
            &this->ui, selector, selected,
            this->selected_type.get(), this->selected_conversion.get(),
            this->local
        );
        *selected = TerrainMap::display_building_info(
            *this->selected_type, *this->selected_conversion, *local
        );
    }

    static void place_building(
        u64 tile_x, u64 tile_z, Terrain& terrain, ComplexBank& complexes,
        Building::Type type, const Building::TypeInfo& type_info,
        const std::vector<Conversion>& conversions
    ) {
        u64 chunk_x = tile_x / terrain.tiles_per_chunk();
        u64 chunk_z = tile_z / terrain.tiles_per_chunk();
        Terrain::ChunkData& chunk = terrain.chunk_at(chunk_x, chunk_z);
        std::optional<ComplexId> complex_id = std::nullopt;
        if(conversions.size() > 0) {
            complex_id = complexes.closest_to(tile_x, tile_z);
            if(!complex_id.has_value()) {
                complex_id = complexes.create_complex();
            } else {
                const Complex& nearest_complex = complexes.get(*complex_id);
                f64 distance = nearest_complex.distance_to(tile_x, tile_z);
                if(distance > Complex::max_building_dist) {
                    complex_id = complexes.create_complex();
                }
            }
            Complex& complex = complexes.get(*complex_id);
            complex.add_member(tile_x, tile_z, Complex::Member(conversions));
        }
        chunk.buildings.push_back({
            type, 
            (u8) (tile_x % terrain.tiles_per_chunk()), 
            (u8) (tile_z % terrain.tiles_per_chunk()),
            complex_id
        });
        i64 end_x = (i64) (tile_x + type_info.width);
        i64 end_z = (i64) (tile_z + type_info.height);
        for(i64 u_tile_x = (i64) tile_x; u_tile_x < end_x; u_tile_x += 1) {
            for(i64 u_tile_z = (i64) tile_z; u_tile_z < end_z; u_tile_z += 1) {
                terrain.remove_foliage_at(u_tile_x, u_tile_z);
            }
        }
    }

    void ConstructionMode::update(
        const engine::Window& window, engine::Scene& scene, 
        const Renderer& renderer
    ) {
        (void) scene;
        (void) renderer;
        const Building::TypeInfo& type_info = Building::types
            .at((size_t) *this->selected_type);
        auto [tile_x, tile_z] = this->terrain.find_selected_terrain_tile(
            window.cursor_pos_ndc(), renderer,
            Vec<3>(type_info.width / 2.0, 0, type_info.height / 2.0)
        );
        this->selected_x = tile_x;
        this->selected_z = tile_z;
        this->placement_valid = this->terrain.valid_building_location(
            (i64) tile_x, (i64) tile_z, this->player.position, type_info
        );
        bool attempted = window.was_pressed(engine::Button::Left)
            && !this->ui.was_clicked();
        if(attempted && this->placement_valid) {
            i64 unemployment = this->terrain.compute_unemployment();
            bool allowed = unemployment >= (i64) type_info.workers
                && this->balance.pay_coins(type_info.cost, this->toasts);
            if(allowed) {
                place_building(
                    tile_x, tile_z, this->terrain, this->complexes,
                    *this->selected_type, type_info, *this->selected_conversion
                );
                this->carriages.refind_all_paths(
                    this->complexes, this->terrain, this->toasts
                );
            } else if(unemployment < (i64) type_info.workers) {
                this->toasts.add_error("toast_missing_unemployment", {
                    std::to_string(unemployment), 
                    std::to_string(type_info.workers)
                });
            }
        }
        if(attempted && !this->placement_valid) {
            this->toasts.add_error("toast_invalid_building_placement", {});
        }
    }

    void ConstructionMode::render(
        const engine::Window& window, engine::Scene& scene, 
        const Renderer& renderer
    ) {
        const engine::Texture& wireframe_texture = this->placement_valid
            ? scene.get<engine::Texture>(ActionMode::wireframe_valid_texture)
            : scene.get<engine::Texture>(ActionMode::wireframe_error_texture);
        u64 chunk_x = this->selected_x / this->terrain.tiles_per_chunk();
        u64 chunk_z = this->selected_z / this->terrain.tiles_per_chunk();
        u64 chunk_rel_x = this->selected_x % this->terrain.tiles_per_chunk();
        u64 chunk_rel_z = this->selected_z % this->terrain.tiles_per_chunk();
        Building building = (Building) { 
            *this->selected_type, 
            (u8) chunk_rel_x, (u8) chunk_rel_z, 
            std::nullopt 
        };
        const Building::TypeInfo& type_info = building.get_type_info();
        Mat<4> transform = this->terrain
            .building_transform(building, chunk_x, chunk_z);
        type_info.render_buildings(
            window, scene, renderer,
            std::array { transform }, true, &wireframe_texture
        );
    }



    static const f64 demolition_refund_factor = 0.25;

    void DemolitionMode::update(
        const engine::Window& window, engine::Scene& scene, 
        const Renderer& renderer
    ) {
        (void) scene;
        auto [tile_x, tile_z] = this->terrain.find_selected_terrain_tile(
            window.cursor_pos_ndc(), renderer, Vec<3>(0.5, 0, 0.5)
        );
        this->selected_tile_x = tile_x;
        this->selected_tile_z = tile_z;
        this->selected = terrain.building_at(
            (i64) tile_x, (i64) tile_z, 
            &this->selected_chunk_x, &this->selected_chunk_z
        );
        bool attempted = this->selected != nullptr
            && window.was_pressed(engine::Button::Left)
            && !this->ui.was_clicked();
        if(attempted && !this->selected->get_type_info().destructible) {
            this->toasts.add_error("toast_indestructible", {});
        } else if(attempted) {
            const Building::TypeInfo& type_info = this->selected->get_type_info();
            i64 unemployment = this->terrain.compute_unemployment();
            bool allowed = unemployment >= (i64) type_info.residents;
            if(allowed) {
                Terrain::ChunkData& chunk = this->terrain
                .chunk_at(this->selected_chunk_x, this->selected_chunk_z);
                if(this->selected->complex.has_value()) {
                    u64 actual_x = this->selected->x
                        + this->selected_chunk_x * this->terrain.tiles_per_chunk();
                    u64 actual_z = this->selected->z
                        + this->selected_chunk_z * this->terrain.tiles_per_chunk();
                    Complex& complex = this->complexes.get(*selected->complex);
                    complex.remove_member(actual_x, actual_z);
                    if(complex.member_count() == 0) {
                        this->complexes.delete_complex(*selected->complex);
                    }
                }
                size_t index = this->selected - chunk.buildings.data();
                chunk.buildings.erase(chunk.buildings.begin() + index);
                this->selected = nullptr;
                u64 refunded = (u64) ((f64) type_info.cost * demolition_refund_factor);
                this->balance.add_coins(refunded, this->toasts);
                this->terrain.reload_chunk_at(
                    this->selected_chunk_x, this->selected_chunk_z
                );
                this->carriages.refind_all_paths(
                    this->complexes, this->terrain, this->toasts
                );
            } else {
                this->toasts.add_error("toast_missing_unemployment", {
                    std::to_string(unemployment), 
                    std::to_string(type_info.residents)
                });
            }
        }
    }

    void DemolitionMode::render(
        const engine::Window& window, engine::Scene& scene, 
        const Renderer& renderer
    ) {
        if(this->selected == nullptr) { return; }
        const engine::Texture& wireframe_texture = scene
            .get<engine::Texture>(ActionMode::wireframe_error_texture);
        const Building::TypeInfo& type_info = this->selected->get_type_info();
        Mat<4> transform = this->terrain.building_transform(
            *this->selected, this->selected_chunk_x, this->selected_chunk_z
        );
        type_info.render_buildings(
            window, scene, renderer,
            std::array { transform }, true, &wireframe_texture
        );
    }



    static bool valid_path_location(
        u64 tile_x, u64 tile_z, Terrain& terrain
    ) {
        return terrain.elevation_at(tile_x, tile_z) >= 0
            && terrain.elevation_at(tile_x + 1, tile_z) >= 0
            && terrain.elevation_at(tile_x, tile_z + 1) >= 0
            && terrain.elevation_at(tile_x + 1, tile_z + 1) >= 0;
    }

    static const u64 path_placement_cost = 10;
    static const u64 path_removal_refund = 5;

    void PathingMode::update(
        const engine::Window& window, engine::Scene& scene, 
        const Renderer& renderer
    ) {
        (void) scene;
        auto [tile_x, tile_z] = this->terrain.find_selected_terrain_tile(
            window.cursor_pos_ndc(), renderer, Vec<3>(0.5, 0, 0.5)
        );
        this->selected_tile_x = tile_x;
        this->selected_tile_z = tile_z;
        u64 chunk_x = tile_x / this->terrain.tiles_per_chunk();
        u64 chunk_z = tile_z / this->terrain.tiles_per_chunk();
        u64 rel_x = tile_x % this->terrain.tiles_per_chunk();
        u64 rel_z = tile_z % this->terrain.tiles_per_chunk();
        Terrain::ChunkData& chunk = this->terrain.chunk_at(chunk_x, chunk_z);
        bool has_path = chunk.path_at(rel_x, rel_z);
        bool place_path = !has_path 
            && window.was_pressed(engine::Button::Left)
            && !this->ui.was_clicked()
            && valid_path_location(tile_x, tile_z, this->terrain)
            && this->balance.pay_coins(path_placement_cost, this->toasts);
        if(place_path) {
            chunk.set_path_at(rel_x, rel_z, true);
            this->terrain.remove_foliage_at((i64) tile_x, (i64) tile_z);
            this->terrain.reload_chunk_at(chunk_x, chunk_z);
            this->carriages.refind_all_paths(
                this->complexes, this->terrain, this->toasts
            );
        } else if(has_path && window.was_pressed(engine::Button::Right)) {
            chunk.set_path_at(rel_x, rel_z, false);
            this->terrain.reload_chunk_at(chunk_x, chunk_z);
            this->carriages.refind_all_paths(
                this->complexes, this->terrain, this->toasts
            );
            this->balance.add_coins(path_removal_refund, this->toasts);
        }
    }

}