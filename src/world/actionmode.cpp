
#include "actionmode.hpp"
#include "terrainmap.hpp"
#include <iostream>

namespace houseofatmos::world {

    namespace ui = houseofatmos::engine::ui;



    static const u64 carr_spawn_d = 1;

    static void summon_carriage(
        engine::Scene& scene, World& world, engine::Speaker& speaker,
        u64 stable_cx, u64 stable_cz, Toasts& toasts, StatefulRNG& rng,
        Carriage::CarriageType carriage_type
    ) {
        Vec<3> pos;
        bool found_pos = false;
        const Building::TypeInfo& stable
            = Building::types().at((size_t) Building::Stable);
        u64 stable_x = stable_cx - (stable.width / 2);
        u64 stable_z = stable_cz - (stable.height / 2);
        i64 start_x = stable_x - carr_spawn_d;
        i64 start_z = stable_z - carr_spawn_d;
        i64 end_x = stable_x + stable.width + carr_spawn_d;
        i64 end_z = stable_z + stable.height + carr_spawn_d;
        for(i64 x = start_x; x < end_x; x += 1) {
            for(i64 z = start_z; z < end_z; z += 1) {
                if(x < 0 || (u64) x >= world.terrain.width_in_tiles()) { continue; }
                if(z < 0 || (u64) z >= world.terrain.height_in_tiles()) { continue; }
                u64 chunk_x = (u64) x / world.terrain.tiles_per_chunk();
                u64 chunk_z = (u64) z / world.terrain.tiles_per_chunk();
                const Terrain::ChunkData& chunk = world.terrain
                    .chunk_at(chunk_x, chunk_z);
                u64 rel_x = (u64) x % world.terrain.tiles_per_chunk();
                u64 rel_z = (u64) z % world.terrain.tiles_per_chunk();
                bool is_obstacle = !chunk.path_at(rel_x, rel_z)
                    || world.terrain.building_at(x, z) != nullptr;
                if(is_obstacle) { continue; }
                pos = Vec<3>(x + 0.5, 0, z + 0.5) * world.terrain.units_per_tile();
                pos.y() = world.terrain.elevation_at(pos);
                found_pos = true;
                break;
            }
        }
        if(!found_pos) {
            toasts.add_error("toast_no_valid_carriage_location", {});
            return;
        }
        u64 cost = Carriage::carriage_types().at((size_t) carriage_type).cost;
        if(!world.balance.pay_coins(cost, toasts)) { return; }
        world.carriages.agents.push_back(
            Carriage(carriage_type, pos, rng)
        );
        speaker.position = pos;
        speaker.play(scene.get(sound::horse));
    }

    static void summon_locomotive(
        World& world, u64 depot_cx, u64 depot_cz, Toasts& toasts,
        Train::LocomotiveType loco_type
    ) {
        Vec<3> pos;
        bool found_pos = false;
        const Building::TypeInfo& stable
            = Building::types().at((size_t) Building::Stable);
        i64 start_x = depot_cx - stable.width / 2;
        i64 end_x = depot_cx + stable.width / 2;
        i64 z = depot_cz + stable.height / 2 + 1;
        for(i64 x = start_x; x < end_x; x += 1) {
            if(x < 0 || (u64) x >= world.terrain.width_in_tiles()) { continue; }
            if(z < 0 || (u64) z >= world.terrain.height_in_tiles()) { continue; }
            bool has_track = world.terrain.track_pieces_at(x, z) > 0;
            if(!has_track) { continue; }
            pos = Vec<3>(x + 0.5, 0, z + 0.5) * world.terrain.units_per_tile();
            pos.y() = world.terrain.elevation_at(pos);
            found_pos = true;
            break;
        }
        if(!found_pos) {
            toasts.add_error("toast_no_valid_train_location", {});
            return;
        }
        u64 cost = Train::locomotive_types().at((size_t) loco_type).cost;
        if(!world.balance.pay_coins(cost, toasts)) { return; }
        world.trains.agents.push_back(Train(loco_type, pos));
    }

    template<typename C>
    static ui::Element create_agent_selector(
        const engine::Localization& local,
        std::string_view local_title, 
        std::span<const C> choices,
        std::string_view (*local_choice_name)(const C&),
        const ui::Background* (*choice_icon)(const C&),
        std::function<void (const C&, size_t)> handler
    ) {
        const std::string& title = local.text(local_title);
        ui::Element selector = TerrainMap::create_selection_container(title)
            .with_pos(0.95, 0.5, ui::position::window_fract)
            .as_movable();
        for(size_t ci = 0; ci < choices.size(); ci += 1) {
            const C& choice = choices[ci];
            selector.children.push_back(TerrainMap::create_selection_item(
                choice_icon(choice), local.text(local_choice_name(choice)),
                false,
                [handler, choice = &choice, ci]() { handler(*choice, ci); }
            ));
        }
        return selector;
    }

    static const f64 max_agent_summon_dist = 5; // in tiles

    void DefaultMode::update(
        const engine::Window& window, engine::Scene& scene, 
        const Renderer& renderer
    ) {
        (void) renderer;
        this->speaker.update();
        if(!this->permitted) { return; }
        auto [s_tile_x, s_tile_z] = this->world->terrain.find_selected_terrain_tile(
            window.cursor_pos_ndc(), renderer, Vec<3>(0.5, 0, 0.5)
        );
        u64 s_chunk_x, s_chunk_z;
        Building* s_building = this->world->terrain.building_at(
            (i64) s_tile_x, (i64) s_tile_z, &s_chunk_x, &s_chunk_z
        );
        Vec<2> player_tile = this->world->player.character.position
            .swizzle<2>("xz") / this->world->terrain.units_per_tile();
        Vec<2> origin_tile 
            = Vec<2>(this->selector.origin_x, this->selector.origin_z);
        f64 player_dist = (player_tile - origin_tile).len();
        bool too_far = player_dist > max_agent_summon_dist
            && !this->selector.element->hidden;
        bool clicked_building = window.was_pressed(engine::Button::Left)
            && !this->ui.was_clicked()
            && s_building != nullptr;
        if(clicked_building) {
            const Building::TypeInfo& building_type = Building::types()
                .at((size_t) s_building->type);
            u64 asx = s_chunk_x * this->world->terrain.tiles_per_chunk() 
                + s_building->x + building_type.width / 2;
            u64 asz = s_chunk_z * this->world->terrain.tiles_per_chunk() 
                + s_building->z + building_type.height / 2;
            switch(s_building->type) {
                case Building::Stable:
                    this->selector.origin_x = asx;
                    this->selector.origin_z = asz;
                    *this->selector.element 
                        = create_agent_selector<Carriage::CarriageTypeInfo>(
                        this->local, "ui_carriage", Carriage::carriage_types(),
                        [](auto ct) { return ct.local_name; },
                        [](auto ct) { return ct.icon; },
                        [scene = &scene, this, asx, asz](auto ct, auto ti) {
                            (void) ct;
                            summon_carriage(
                                *scene, *this->world, this->speaker, 
                                asx, asz, this->toasts, this->rng,
                                (Carriage::CarriageType) ti
                            );
                        }
                    );
                    break;
                case Building::TrainDepot: {
                    this->selector.origin_x = asx;
                    this->selector.origin_z = asz;
                    *this->selector.element 
                        = create_agent_selector<Train::LocomotiveTypeInfo>(
                        this->local, "ui_train", Train::locomotive_types(),
                        [](auto ct) { return ct.local_name; },
                        [](auto ct) { return ct.icon; },
                        [this, asx, asz](auto ct, auto ti) {
                            (void) ct;
                            summon_locomotive(
                                *this->world, asx, asz, this->toasts,
                                (Train::LocomotiveType) ti
                            );
                        }
                    );
                    break;
                }
                default: break;
            }
        }
        bool close_menu = window.was_pressed(engine::Button::Left)
            && !clicked_building;
        if(close_menu || too_far) {
            this->selector.element->hidden = true;
        }
    }



    static Vec<3> tile_bounded_position(
        u64 min_x, u64 min_z, u64 max_x, u64 max_z,
        const Vec<3>& pos, const Terrain& terrain
    ) {
        Vec<3> min = Vec<3>(min_x, 0, min_z) * terrain.units_per_tile();
        Vec<3> max = Vec<3>(max_x, 0, max_z) * terrain.units_per_tile();
        Vec<3> result = Vec<3>(
            std::min(std::max(pos.x(), min.x()), max.x()),
            0,
            std::min(std::max(pos.z(), min.z()), max.z())
        );
        result.y() = terrain.elevation_at(result);
        return result;
    }



    TerraformMode::TerraformMode(ActionContext ctx): ActionMode(ctx) {
        this->has_selection = false;
    }

    void TerraformMode::init_ui() {
        this->ui.with_element(ui::Element()
            .with_pos(50, 15, ui::position::window_bl_units)
            .with_size(0, 0, ui::size::units_with_children)
            .with_background(&ui_background::note)
            .with_list_dir(ui::Direction::Horizontal)
            .as_movable()
        );
        this->mode_selection = &this->ui.root.children.back();
        this->ui.with_element(ui::Element()
            .as_phantom()
            .with_pos(0.0, 0.0, ui::position::window_tl_units)
            .with_size(1.0, 1.0, ui::size::window_fract)
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
                .with_size(16, 16, ui::size::units)
                .with_background(TerraformMode::mode_icons[mode_i])
                .with_click_handler([this, mode_i]() {
                    if(mode_i == (u64) this->mode) { return; }
                    this->set_mode((Mode) mode_i);
                })
                .with_padding(0)
                .with_background(
                    is_selected? &ui_background::border_selected
                        : &ui_background::border,
                    is_selected? &ui_background::border_selected
                        : &ui_background::border_hovering
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



    using BuildingVariant = ConstructionMode::BuildingVariant;
    struct BuildingGroup {
        Building::Type type;
        std::vector<BuildingVariant> variants;

        BuildingGroup(Building::Type type, std::vector<BuildingVariant>&& variants) {
            this->type = type;
            this->variants = std::move(variants);
        }
    };
    static const std::vector<BuildingGroup> buildable = {
        BuildingGroup(Building::House, {}),
        BuildingGroup(Building::Farmland, {
            { 
                { Conversion(
                    {}, 
                    { { 8, Item::Wheat } }, 
                    8.0 
                ) }
            },
            { 
                { Conversion(
                    {}, 
                    { { 8, Item::Barley } }, 
                    8.0 
                ) }
            },
            { 
                { Conversion(
                    {}, 
                    { { 8, Item::Hops } }, 
                    8.0 
                ) }
            }
        }),
        BuildingGroup(Building::Mineshaft, {
            { 
                { Conversion( 
                    {}, 
                    { { 1, Item::Coal } }, 
                    1.0 
                ) },
                Resource::Type::Coal 
            },
            { 
                { Conversion( 
                    {}, 
                    { { 1, Item::CrudeOil } }, 
                    1.0 
                ) },
                Resource::Type::CrudeOil 
            },
            { 
                { Conversion( 
                    {}, 
                    { { 1, Item::Salt } }, 
                    1.0 
                ) },
                Resource::Type::Salt 
            },
            { 
                { Conversion(  
                    {}, 
                    { { 1, Item::IronOre } }, 
                    1.0 
                ) },
                Resource::Type::IronOre 
            },
            { 
                { Conversion(  
                    {}, 
                    { { 1, Item::CopperOre } }, 
                    1.0 
                ) },
                Resource::Type::CopperOre 
            },
            { 
                { Conversion( 
                    {}, 
                    { { 1, Item::ZincOre } }, 
                    1.0 
                ) },
                Resource::Type::ZincOre 
            }
        }),
        BuildingGroup(Building::Windmill, {
            { 
                { Conversion( 
                    { { 4, Item::Wheat } }, 
                    { { 1, Item::Flour } }, 
                    1.0 
                ) }
            },
            { 
                { Conversion( 
                    { { 4, Item::Barley } }, 
                    { { 1, Item::Malt } }, 
                    1.0 
                ) }
            }
        }),
        BuildingGroup(Building::Factory, {
            { 
                { Conversion( 
                    { { 4, Item::Yarn } }, 
                    { { 1, Item::Fabric } }, 
                    4.0
                ) },
                std::nullopt,
                research::Research::RewardFabric
            },
            { 
                { Conversion( 
                    { { 1, Item::Cattle } }, 
                    { { 8, Item::Milk } }, 
                    4.0
                ) }
            },
            { 
                { Conversion( 
                    { { 1, Item::Cattle } }, 
                    { { 4, Item::Beef } }, 
                    4.0
                ) }
            },
            { 
                { Conversion( 
                    { { 1, Item::Cattle } }, 
                    { { 2, Item::Leather } }, 
                    8.0
                ) }
            },
            { 
                { Conversion( 
                    { { 2, Item::IronOre }, { 1, Item::Coal } }, 
                    { { 2, Item::Steel } }, 
                    4.0 
                ) },
                std::nullopt,
                research::Research::RewardSteel
            },
            { 
                { Conversion( 
                    { { 1, Item::Wood } }, 
                    { { 4, Item::Planks } }, 
                    4.0 
                ) },
                std::nullopt,
                research::Research::RewardPlanks
            },
            { 
                { Conversion( 
                    { { 2, Item::CopperOre }, { 1, Item::ZincOre }, { 1, Item::Coal } }, 
                    { { 3, Item::Brass } }, 
                    4.0 
                ) }
            },
            { 
                { Conversion( 
                    { { 1, Item::Brass } }, 
                    { { 4, Item::BrassRods } }, 
                    2.0 
                ) }
            },
            { 
                { Conversion( 
                    { { 1, Item::Brass } }, 
                    { { 1, Item::BrassPlates } }, 
                    2.0
                ) }
            },
            { 
                { Conversion( 
                    { { 1, Item::BrassPlates }, { 4, Item::BrassRods } }, 
                    { { 4, Item::BrassGears } }, 
                    2.0
                ) }
            },
            { 
                { Conversion( 
                    { { 1, Item::Wood } }, 
                    { { 1, Item::Coal } }, 
                    4.0
                ) }
            },
            { 
                { Conversion( 
                    { { 2, Item::Fabric } }, 
                    { { 1, Item::Clothing } }, 
                    4.0
                ) }
            },
            { 
                { Conversion( 
                    { { 8, Item::Milk }, { 1, Item::Salt } }, 
                    { { 4, Item::Cheese } }, 
                    8.0
                ) },
                std::nullopt,
                research::Research::RewardCheese
            },
            { 
                { Conversion( 
                    { { 4, Item::Beef }, { 1, Item::Salt }, { 1, Item::Coal } }, 
                    { { 4, Item::Steak } }, 
                    4.0
                ) },
                std::nullopt,
                research::Research::RewardSteak
            },
            { 
                { Conversion( 
                    { { 2, Item::CrudeOil }, { 1, Item::Coal } }, 
                    { { 2, Item::Oil } }, 
                    4.0
                ) },
                std::nullopt,
                research::Research::RewardOil
            },
            { 
                { Conversion( 
                    { { 4, Item::Malt }, { 2, Item::Hops }, { 1, Item::Coal } }, 
                    { { 4, Item::Beer } }, 
                    8.0
                ) },
                std::nullopt,
                research::Research::RewardBeer
            },
            { 
                { Conversion( 
                    { { 1, Item::Leather }, { 3, Item::Steel } }, 
                    { { 1, Item::Armor } }, 
                    4.0
                ) }
            },
            { 
                { Conversion( 
                    { { 4, Item::Flour }, { 1, Item::Coal } }, 
                    { { 4, Item::Bread } }, 
                    4.0
                ) }
            },
            { 
                { Conversion( 
                    { { 1, Item::Planks }, { 2, Item::Steel } }, 
                    { { 1, Item::Tools } }, 
                    4.0
                ) }
            },
            { 
                { Conversion( 
                    { { 4, Item::BrassPlates } }, 
                    { { 1, Item::BrassPots } }, 
                    4.0
                ) },
                std::nullopt,
                research::Research::RewardBrassPots
            },
            { 
                { Conversion( 
                    { { 1, Item::BrassPlates }, { 1, Item::BrassRods } }, 
                    { { 1, Item::OilLanterns } }, 
                    4.0
                ) },
                std::nullopt,
                research::Research::RewardOilLanterns
            },
            { 
                { Conversion( 
                    { { 1, Item::BrassPlates }, { 4, Item::BrassGears } }, 
                    { { 1, Item::Watches } }, 
                    4.0
                ) },
                std::nullopt,
                research::Research::RewardWatches
            },
            { 
                { Conversion( 
                    { { 2, Item::Steel } }, 
                    { { 1, Item::SteelBeams } }, 
                    4.0
                ) }
            },
            { 
                { Conversion( 
                    { { 2, Item::Planks }, { 4, Item::BrassGears } }, 
                    { { 1, Item::PowerLooms } }, 
                    4.0
                ) }
            },
            {
                { Conversion( 
                    { { 4, Item::BrassPlates }, { 4, Item::BrassGears } }, 
                    { { 1, Item::SteamEngines } }, 
                    4.0
                ) }
            }
        }),
        BuildingGroup(Building::Pasture, {
            { 
                { Conversion( 
                    { { 4, Item::Wheat } }, 
                    { { 1, Item::Yarn } }, 
                    1.0 
                ) } 
            },
            { 
                { Conversion( 
                    { { 8, Item::Wheat } }, 
                    { { 1, Item::Cattle } }, 
                    1.0 
                ) } 
            }
        }),
        BuildingGroup(Building::TreeFarm, {
            { 
                { Conversion( 
                    {}, 
                    { { 8, Item::Wood } }, 
                    8.0 
                ) }
            }
        }),
        BuildingGroup(Building::Stable, {}),
        BuildingGroup(Building::TrainDepot, {})
    };

    static ui::Element create_building_selector(
        ui::Manager* ui, ui::Element* dest, ui::Element* selected,
        Building::Type* s_type, 
        const ConstructionMode::BuildingVariant** s_variant,
        const engine::Localization* local, const research::Research* research
    );

    static const size_t max_column_variants = 15;

    static ui::Element create_variant_selector(
        ui::Manager* ui, ui::Element* dest, ui::Element* selected,
        const BuildingGroup& group,
        Building::Type* s_type,
        const ConstructionMode::BuildingVariant** s_variant,
        const engine::Localization* local, const research::Research* research
    ) {
        ui::Element selector 
            = TerrainMap::create_selection_container(
                local->text("ui_product_selection")
            )
            .with_pos(0.95, 0.5, ui::position::window_fract)
            .as_movable();
        ui::Element container = ui::Element()
            .with_size(0, 0, ui::size::units_with_children)
            .with_list_dir(ui::Direction::Horizontal)
            .as_movable();
        size_t column_count = (group.variants.size() / max_column_variants) + 1;
        for(size_t column_i = 0; column_i < column_count; column_i += 1) {
            ui::Element column = ui::Element()
                .with_size(0, 0, ui::size::units_with_children)
                .with_list_dir(ui::Direction::Vertical)
                .as_movable();
            size_t first_var_i = column_i * max_column_variants;
            size_t end_var_i = std::min(
                first_var_i + max_column_variants, group.variants.size()
            );
            for(size_t var_i = first_var_i; var_i < end_var_i; var_i += 1) {
                const auto& variant = group.variants[var_i];
                bool is_unlocked = !variant.required_advancement.has_value()
                    || research->is_unlocked(*variant.required_advancement);
                if(!is_unlocked) { continue; }
                if(variant.conversions.size() == 0) { continue; }
                if(variant.conversions.at(0).outputs.size() == 0) { continue; }
                const Item::TypeInfo& result = Item::types().at(
                    (size_t) variant.conversions.at(0).outputs[0].item
                );
                column.children.push_back(TerrainMap::create_selection_item(
                    result.icon, local->text(result.local_name), false,
                    [
                        ui, dest, selected, s_type, s_variant, local, 
                        type = group.type, variant = &variant, research
                    ]() {
                        *s_type = type;
                        *s_variant = variant;
                        *dest = create_building_selector(
                            ui, dest, selected, s_type, s_variant, 
                            local, research
                        );
                        *selected = TerrainMap::display_building_info(
                            *s_type, variant->conversions, *local
                        );
                    }
                ));
            }
            container.children.push_back(std::move(column));
        }
        selector.children.push_back(std::move(container));
        return selector;
    }   

    static ui::Element create_building_selector(
        ui::Manager* ui, ui::Element* dest, ui::Element* selected,
        Building::Type* s_type, 
        const ConstructionMode::BuildingVariant** s_variant,
        const engine::Localization* local, const research::Research* research
    ) {
        ui::Element selector 
            = TerrainMap::create_selection_container(
                local->text("ui_building_selection")
            )
            .with_pos(0.95, 0.5, ui::position::window_fract)
            .as_movable();
        for(const BuildingGroup& group: buildable) {
            const BuildingGroup* group_ptr = &group;
            const Building::TypeInfo& type = Building::types()
                .at((size_t) group.type);
            selector.children.push_back(TerrainMap::create_selection_item(
                type.icon, local->text(type.local_name), *s_type == group.type,
                [
                    ui, dest, selected, s_type, s_variant, local, group_ptr, 
                    research
                ]() {
                    std::vector<size_t> unlocked_variants;
                    size_t locked_var_c = group_ptr->variants.size();
                    for(size_t var_i = 0; var_i < locked_var_c; var_i += 1) {
                        const ConstructionMode::BuildingVariant& var
                            = group_ptr->variants[var_i];
                        bool is_unlocked = !var.required_advancement.has_value()
                            || research->is_unlocked(*var.required_advancement);
                        if(is_unlocked) { unlocked_variants.push_back(var_i); }
                    }
                    if(group_ptr->variants.size() == 0) {
                        *s_type = group_ptr->type;
                        *s_variant = nullptr;
                        *selected = TerrainMap::display_building_info(
                            *s_type, std::span<Conversion>(), *local
                        );
                        *dest = create_building_selector(
                            ui, dest, selected, s_type, s_variant, local, 
                            research
                        );
                        return;
                    }
                    if(unlocked_variants.size() == 0) {
                        *s_type = Building::Type::House;
                        *s_variant = nullptr;
                        *selected = TerrainMap::display_building_info(
                            *s_type, std::span<Conversion>(), *local
                        );
                        *dest = create_building_selector(
                            ui, dest, selected, s_type, s_variant, local, 
                            research
                        );
                        return;
                    }
                    if(unlocked_variants.size() == 1) {
                        *s_type = group_ptr->type;
                        *s_variant = &group_ptr->variants[unlocked_variants[0]];
                        *selected = TerrainMap::display_building_info(
                            *s_type, (**s_variant).conversions, *local
                        );
                        *dest = create_building_selector(
                            ui, dest, selected, s_type, s_variant, local,
                            research
                        );
                        return;
                    }
                    *dest = create_variant_selector(
                        ui, dest, selected, *group_ptr, s_type, s_variant, 
                        local, research
                    );
                }
            ));
        }
        return selector;
    }

    ConstructionMode::ConstructionMode(
        ActionContext ctx
    ): ActionMode(ctx) {
        this->selected_x = 0;
        this->selected_z = 0;
        this->selected_type = std::make_unique<Building::Type>(Building::House);
        this->selected_variant = std::make_unique<const BuildingVariant*>();
        this->placement_valid = false;
    }

    void ConstructionMode::init_ui() {
        this->ui.root.children.push_back(ui::Element().as_phantom().as_movable());
        ui::Element* selector = &this->ui.root.children.back();
        this->ui.root.children.push_back(ui::Element().as_phantom().as_movable());
        ui::Element* selected = &this->ui.root.children.back();
        *selector = create_building_selector(
            &this->ui, selector, selected,
            this->selected_type.get(), this->selected_variant.get(),
            &local, &this->world->research
        );
        *selected = TerrainMap::display_building_info(
            *this->selected_type,
            *this->selected_variant != nullptr
                ? std::span((**this->selected_variant).conversions)
                : std::span<Conversion>(), 
            local
        );
    }

    static const i64 terrain_overlay_r = 1;

    void ConstructionMode::collect_chunk_overlays(
        i64 viewed_chunk_x, i64 viewed_chunk_z
    ) {
        bool matches_current = viewed_chunk_x == this->last_viewed_chunk_x
            && viewed_chunk_z == this->last_viewed_chunk_z;
        if(matches_current) { return; }
        this->last_viewed_chunk_x = viewed_chunk_x;
        this->last_viewed_chunk_z = viewed_chunk_z;
        this->chunk_overlays.clear();
        u64 s_chunk_x = (u64) std::max(
            (i64) viewed_chunk_x - terrain_overlay_r, (i64) 0
        );
        u64 s_chunk_z = (u64) std::max(
            (i64) viewed_chunk_z - terrain_overlay_r, (i64) 0
        );
        u64 e_chunk_x = std::min(
            s_chunk_x + (u64) (terrain_overlay_r * 2), 
            this->world->terrain.width_in_chunks() - 1
        );
        u64 e_chunk_z = std::min(
            s_chunk_z + (u64) (terrain_overlay_r * 2), 
            this->world->terrain.height_in_chunks() - 1
        );
        for(u64 chunk_x = s_chunk_x; chunk_x <= e_chunk_x; chunk_x += 1) {
            for(u64 chunk_z = s_chunk_z; chunk_z <= e_chunk_z; chunk_z += 1) {
                this->chunk_overlays.push_back({
                    chunk_x, chunk_z,
                    this->world->terrain
                        .build_chunk_terrain_geometry(chunk_x, chunk_z)
                });
            }
        }
    }

    static bool valid_building_location(
        Terrain& terrain,
        i64 tile_x, i64 tile_z, const Vec<3>& player_position, 
        const Building::TypeInfo& building_type
    ) {
        i64 player_tile_x = (i64) player_position.x() / terrain.units_per_tile();
        i64 player_tile_z = (i64) player_position.z() / terrain.units_per_tile();
        i64 end_x = tile_x + (i64) building_type.width;
        i64 end_z = tile_z + (i64) building_type.height;
        if(tile_x < 0 || (u64) end_x > terrain.width_in_tiles()) { return false; }
        if(tile_z < 0 || (u64) end_z > terrain.height_in_tiles()) { return false; }
        i16 target = terrain.elevation_at((u64) tile_x, (u64) tile_z);
        if(target < 0) { return false; }
        for(i64 x = tile_x; x <= end_x; x += 1) {
            for(i64 z = tile_z; z <= end_z; z += 1) {
                i16 elevation = terrain.elevation_at((u64) x, (u64) z);
                if(elevation != target) { return false; }
            }
        }
        for(i64 x = tile_x; x < end_x; x += 1) {
            for(i64 z = tile_z; z < end_z; z += 1) {
                if(x == player_tile_x && z == player_tile_z) { return false; }
                if(terrain.building_at(x, z) != nullptr) { return false; }
                if(terrain.bridge_at(x, z) != nullptr) { return false; }
                if(terrain.track_pieces_at(x, z) > 0) { return false; }
            }
        }
        return true;
    }

    static bool required_resource_present(
        Terrain& terrain,
        i64 tile_x, i64 tile_z,
        const Building::TypeInfo& building_type,
        const ConstructionMode::BuildingVariant* building_variant
    ) {
        bool requires_resource = building_variant != nullptr
            && building_variant->required_resource.has_value();
        if(!requires_resource) { return true; }
        for(i64 x = tile_x; x < tile_x + (i64) building_type.width; x += 1) {
            for(i64 z = tile_z; z < tile_z +(i64)  building_type.height; z += 1) {
                const Resource* resource = terrain.resource_at(x, z);
                bool is_present = resource != nullptr
                    && resource->type == *building_variant->required_resource;
                if(is_present) { return true; }
            }
        }
        return false;
    }

    static void place_building(
        u64 tile_x, u64 tile_z, Terrain& terrain, ComplexBank& complexes,
        Building::Type type, const Building::TypeInfo& type_info,
        const ConstructionMode::BuildingVariant* building_variant
    ) {
        u64 chunk_x = tile_x / terrain.tiles_per_chunk();
        u64 chunk_z = tile_z / terrain.tiles_per_chunk();
        Terrain::ChunkData& chunk = terrain.chunk_at(chunk_x, chunk_z);
        std::optional<ComplexId> complex_id = std::nullopt;
        if(building_variant != nullptr) {
            complex_id = complexes.closest_to(tile_x, tile_z);
            if(!complex_id.has_value()) {
                complex_id = complexes.create_complex();
            } else {
                const Complex& nearest_complex = complexes.get(*complex_id);
                // find distances of nearest and farthest buildings in complex
                f64 near_distance = nearest_complex.distance_to(tile_x, tile_z);
                f64 far_distance = nearest_complex
                    .farthest_distance_to(tile_x, tile_z);
                bool join_existing = near_distance <= Complex::max_building_dist
                    && far_distance <= Complex::max_diameter;
                if(!join_existing) {
                    complex_id = complexes.create_complex();
                }
            }
            Complex& complex = complexes.get(*complex_id);
            complex.add_member(
                tile_x, tile_z, Complex::Member(building_variant->conversions)
            );
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
        this->speaker.update();
        if(!this->permitted) { return; }
        const Building::TypeInfo& type_info 
            = Building::types().at((size_t) *this->selected_type);
        auto [tile_x, tile_z] = this->world->terrain.find_selected_terrain_tile(
            window.cursor_pos_ndc(), renderer,
            Vec<3>(type_info.width / 2.0, 0, type_info.height / 2.0)
        );
        this->selected_x = tile_x;
        this->selected_z = tile_z;
        this->placement_valid = valid_building_location(
            this->world->terrain,
            (i64) tile_x, (i64) tile_z, this->world->player.character.position, 
            type_info
        );
        bool attempted = window.was_pressed(engine::Button::Left)
            && !this->ui.was_clicked();
        if(attempted && this->placement_valid) {
            i64 unemployment = this->world->terrain.compute_unemployment();
            bool has_resource = required_resource_present(
                this->world->terrain, tile_x, tile_z,
                type_info, *this->selected_variant
            );
            bool allowed = unemployment >= (i64) type_info.workers
                && has_resource
                && this->world->balance.pay_coins(type_info.cost, this->toasts);
            if(allowed) {
                place_building(
                    tile_x, tile_z, this->world->terrain, this->world->complexes,
                    *this->selected_type, type_info, *this->selected_variant
                );
                this->world->carriages.find_paths(&this->toasts);
                this->speaker.position = tile_bounded_position(
                    tile_x, tile_z, 
                    tile_x + type_info.width, tile_z + type_info.height,
                    this->world->player.character.position,
                    this->world->terrain
                );
                this->speaker.play(scene.get(sound::build));
            } else if(unemployment < (i64) type_info.workers) {
                this->toasts.add_error("toast_missing_unemployment", {
                    std::to_string(unemployment), 
                    std::to_string(type_info.workers)
                });
            } else if(!has_resource) {
                Resource::Type r_resource = *(**this->selected_variant)
                    .required_resource;
                this->toasts.add_error("toast_building_requires_resource", {
                    local.text(
                        Resource::types().at((size_t) r_resource).local_name
                    ), 
                });
            }
        }
        if(attempted && !this->placement_valid) {
            this->toasts.add_error("toast_invalid_building_placement", {});
        }
    }

    void ConstructionMode::render(
        const engine::Window& window, engine::Scene& scene, 
        Renderer& renderer
    ) {
        if(!this->permitted) { return; }
        this->collect_chunk_overlays(
            this->world->terrain.viewed_chunk_x(),
            this->world->terrain.viewed_chunk_z()
        );
        // render overlay
        engine::Shader& terrain_overlay_shader = scene
            .get(ActionMode::terrain_overlay_shader);
        terrain_overlay_shader.set_uniform(
            "u_view_proj", renderer.compute_view_proj()
        );
        for(ChunkOverlay& overlay: this->chunk_overlays) {
            Vec<3> offset = Vec<3>(overlay.x, 0, overlay.z) 
                * this->world->terrain.tiles_per_chunk()
                * this->world->terrain.units_per_tile();
            terrain_overlay_shader.set_uniform(
                "u_transform", Mat<4>::translate(offset)
            );
            overlay.terrain.render(
                terrain_overlay_shader, 
                renderer.output().as_target(), 
                1, 
                engine::FaceCulling::Disabled, 
                engine::Rendering::Surfaces,
                engine::DepthTesting::Disabled
            );
        }
        // render building
        const engine::Texture& wireframe_texture = this->placement_valid
            ? scene.get(ActionMode::wireframe_valid_texture)
            : scene.get(ActionMode::wireframe_error_texture);
        u64 chunk_x = this->selected_x / this->world->terrain.tiles_per_chunk();
        u64 chunk_z = this->selected_z / this->world->terrain.tiles_per_chunk();
        u64 chunk_rel_x = this->selected_x % this->world->terrain.tiles_per_chunk();
        u64 chunk_rel_z = this->selected_z % this->world->terrain.tiles_per_chunk();
        Building building = (Building) { 
            *this->selected_type, 
            (u8) chunk_rel_x, (u8) chunk_rel_z, 
            std::nullopt 
        };
        const Building::TypeInfo& type_info = building.get_type_info();
        Mat<4> transform = this->world->terrain
            .building_transform(building, chunk_x, chunk_z);
        type_info.render_buildings(
            window, scene, renderer,
            std::array { transform },
            engine::Rendering::Wireframe, 
            &wireframe_texture
        );
    }



    static const std::vector<std::optional<research::Research::Advancement>> 
            required_bridge_advancements = {
        std::nullopt, // Bridge::Type::Wooden
        std::nullopt, // Bridge::Type::Stone
        research::Research::RewardSteelBridges
    };

    static ui::Element create_bridge_selector(
        ui::Manager* ui, ui::Element* dest,
        Bridge::Type* s_type,
        const engine::Localization* local, const research::Research* research
    ) {
        ui::Element selector 
            = TerrainMap::create_selection_container(
                local->text("ui_bridge_selection")
            )
            .with_pos(0.95, 0.5, ui::position::window_fract)
            .as_movable();
        for(size_t type_id = 0; type_id < Bridge::types().size(); type_id += 1) {
            auto r_advancement = required_bridge_advancements[type_id];
            bool unlocked = !r_advancement.has_value()
                || research->is_unlocked(*r_advancement);
            if(!unlocked) { continue; }
            const Bridge::TypeInfo& type = Bridge::types().at(type_id);
            selector.children.push_back(TerrainMap::create_selection_item(
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



    void PathingMode::update_overlay() {
        auto overlay = engine::Mesh { { engine::Mesh::F32, 3 } };
        u64 left = this->selected_tile_x;
        u64 right = left + 1;
        u64 top = this->selected_tile_z;
        u64 bottom = top + 1;
        overlay.start_vertex();
        overlay.put_f32({
            (f32) (left * this->world->terrain.units_per_tile()),
            (f32) this->world->terrain.elevation_at(left, top),
            (f32) (top * this->world->terrain.units_per_tile())
        });
        u16 tl = overlay.complete_vertex();
        overlay.start_vertex();
        overlay.put_f32({
            (f32) (right * this->world->terrain.units_per_tile()),
            (f32) this->world->terrain.elevation_at(right, top),
            (f32) (top * this->world->terrain.units_per_tile())
        });
        u16 tr = overlay.complete_vertex();
        overlay.start_vertex();
        overlay.put_f32({
            (f32) (left * this->world->terrain.units_per_tile()),
            (f32) this->world->terrain.elevation_at(left, bottom),
            (f32) (bottom * this->world->terrain.units_per_tile())
        });
        u16 bl = overlay.complete_vertex();
        overlay.start_vertex();
        overlay.put_f32({
            (f32) (right * this->world->terrain.units_per_tile()),
            (f32) this->world->terrain.elevation_at(right, bottom),
            (f32) (bottom * this->world->terrain.units_per_tile())
        });
        u16 br = overlay.complete_vertex();
        // tl---tr
        //  | \ |
        // bl---br
        overlay.add_element(tl, bl, br);
        overlay.add_element(tl, br, tr);
        this->overlay = std::move(overlay);
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
        this->speaker.update();
        if(!this->permitted) { return; }
        auto [tile_x, tile_z] = this->world->terrain.find_selected_terrain_tile(
            window.cursor_pos_ndc(), renderer, Vec<3>(0.5, 0, 0.5)
        );
        bool selected_changed = this->selected_tile_x != tile_x
            || this->selected_tile_z != tile_z;
        this->selected_tile_x = tile_x;
        this->selected_tile_z = tile_z;
        if(selected_changed) { this->update_overlay(); }
        u64 chunk_x = tile_x / this->world->terrain.tiles_per_chunk();
        u64 chunk_z = tile_z / this->world->terrain.tiles_per_chunk();
        u64 rel_x = tile_x % this->world->terrain.tiles_per_chunk();
        u64 rel_z = tile_z % this->world->terrain.tiles_per_chunk();
        Terrain::ChunkData& chunk = this->world->terrain.chunk_at(chunk_x, chunk_z);
        bool has_path = chunk.path_at(rel_x, rel_z);
        bool place_path = !has_path 
            && window.is_down(engine::Button::Left)
            && !this->ui.is_hovered_over()
            && valid_path_location(tile_x, tile_z, this->world->terrain)
            && this->world->balance.pay_coins(path_placement_cost, this->toasts);
        if(place_path) {
            chunk.set_path_at(rel_x, rel_z, true);
            this->world->terrain.remove_foliage_at((i64) tile_x, (i64) tile_z);
            this->world->terrain.reload_chunk_at(chunk_x, chunk_z);
            this->world->carriages.find_paths(&this->toasts);
            this->speaker.position = Vec<3>(tile_x, 0, tile_z)
                * this->world->terrain.units_per_tile()
                + Vec<3>(0, this->world->terrain.elevation_at(tile_x, tile_z), 0);
            this->speaker.play(scene.get(sound::terrain_mod));
        } else if(has_path && window.is_down(engine::Button::Right)) {
            chunk.set_path_at(rel_x, rel_z, false);
            this->world->terrain.reload_chunk_at(chunk_x, chunk_z);
            this->world->carriages.find_paths(&this->toasts);
            this->world->balance.add_coins(path_removal_refund, this->toasts);
            this->speaker.position = Vec<3>(tile_x, 0, tile_z)
                * this->world->terrain.units_per_tile()
                + Vec<3>(0, this->world->terrain.elevation_at(tile_x, tile_z), 0);
            this->speaker.play(scene.get(sound::terrain_mod));
        }
    }

    void PathingMode::render(
        const engine::Window& window, engine::Scene& scene, 
        Renderer& renderer
    ) {
        (void) window;
        if(!this->permitted) { return; }
        if(!this->overlay.has_value()) { return; }
        engine::Shader& path_overlay_shader = scene
            .get(ActionMode::path_overlay_shader);
        path_overlay_shader.set_uniform(
            "u_view_proj", renderer.compute_view_proj()
        );
        path_overlay_shader.set_uniform(
            "u_tile_size", (f32) this->world->terrain.units_per_tile()
        );
        this->overlay->render(
            path_overlay_shader,
            renderer.output().as_target(),
            1,
            engine::FaceCulling::Disabled,
            engine::Rendering::Surfaces,
            engine::DepthTesting::Disabled
        );
    }



    void TrackingMode::init_ui() {
        this->ui.with_element(ui::Element()
            .as_phantom()
            .with_pos(0.0, 0.0, ui::position::window_tl_units)
            .with_size(1.0, 1.0, ui::size::window_fract)
            .as_movable()
        );
        this->track_markers = &this->ui.root.children.back();
    }

    static const i64 track_marker_ch_rad = 1;
    static const f64 max_display_tile_dist = 3;

    void TrackingMode::update_track_markers(
        const engine::Window& window, const Renderer& renderer, 
        u64 tile_x, u64 tile_z, std::optional<Vec<3>>& closest_marker
    ) {
        i64 s_ch_x = (i64) tile_x / (i64) this->world->terrain.tiles_per_chunk()
            - track_marker_ch_rad;
        s_ch_x = std::max(s_ch_x, (i64) 0);
        u64 e_ch_x = s_ch_x + track_marker_ch_rad * 2;
        e_ch_x = std::min(e_ch_x, this->world->terrain.width_in_chunks() - 1);
        i64 s_ch_z = (i64) tile_z / (i64) this->world->terrain.tiles_per_chunk()
            - track_marker_ch_rad;
        s_ch_z = std::max(s_ch_z, (i64) 0);
        u64 e_ch_z = s_ch_z + track_marker_ch_rad * 2;
        e_ch_z = std::min(e_ch_z, this->world->terrain.height_in_chunks() - 1);
        std::vector<Vec<4>> markers;
        for(u64 ch_x = (u64) s_ch_x; ch_x <= e_ch_x; ch_x += 1) {
            for(u64 ch_z = (u64) s_ch_z; ch_z <= e_ch_z; ch_z += 1) {
                const Terrain::ChunkData& chunk = this->world->terrain
                    .chunk_at(ch_x, ch_z);
                for(const TrackPiece& track_piece: chunk.track_pieces) {
                    Mat<4> t = track_piece.build_transform(
                        ch_x, ch_z,
                        this->world->terrain.tiles_per_chunk(), 
                        this->world->terrain.units_per_tile()
                    );
                    const TrackPiece::TypeInfo& piece_type = TrackPiece::types()
                        .at((size_t) track_piece.type);
                    markers.push_back(t * piece_type.points[0].with(1.0));
                    markers.push_back(t * piece_type.points.back().with(1.0));
                }
            }
        }
        this->track_markers->children.clear();
        closest_marker = std::nullopt;
        f64 closest_dist = INFINITY;
        for(const Vec<4>& marker_pos: markers) {
            Vec<3> world = marker_pos.swizzle<3>("xyz");
            f64 units_per_tile = this->world->terrain.units_per_tile();
            Vec<3> tile_offset = Vec<3>(tile_x, 0, tile_z)
                - (world * Vec<3>(1, 0, 1) / units_per_tile);
            if(tile_offset.len() > max_display_tile_dist) { continue; }
            Vec<2> ndc = renderer.world_to_ndc(world);
            this->track_markers->children.push_back(ui::Element()
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
            f64 cursor_dist = (window.cursor_pos_ndc() - ndc).len();
            if(cursor_dist < closest_dist) {
                closest_dist = cursor_dist;
                closest_marker = world;
            }
        }
    }

    static const Vec<2> track_base_dir_ndc = Vec<2>(0, 1); // NDC; up the screen

    void TrackingMode::select_piece_disconnected(
        const engine::Window& window, const Renderer& renderer,
        u64 tile_x, u64 tile_z
    ) {
        this->preview_ch_x = tile_x / this->world->terrain.tiles_per_chunk();
        this->preview_ch_z = tile_z / this->world->terrain.tiles_per_chunk();
        this->preview_piece.x = tile_x % this->world->terrain.tiles_per_chunk();
        this->preview_piece.z = tile_z % this->world->terrain.tiles_per_chunk();
        this->preview_piece.elevation = this->world->terrain
            .elevation_at(tile_x, tile_z);
        f64 center_x = ((f64) tile_x + 0.5) 
            * (f64) this->world->terrain.units_per_tile();
        f64 center_z = ((f64) tile_z + 0.5)
            * (f64) this->world->terrain.units_per_tile();
        Vec<3> center = Vec<3>(
            center_x, (f64) this->preview_piece.elevation, center_z
        );
        Vec<2> center_ndc = renderer.world_to_ndc(center);
        Vec<2> to_cursor = (window.cursor_pos_ndc() - center_ndc).normalized();
        f64 dir_cross = track_base_dir_ndc.x() * to_cursor.y()
            - track_base_dir_ndc.y() * to_cursor.x();
        f64 angle_rad = atan2(dir_cross, track_base_dir_ndc.dot(to_cursor));
        if(angle_rad < 0.0) { angle_rad += pi; }
        f64 angle_rot = angle_rad / (2.0 * pi); // angle in number of rotations
        if(angle_rot >= 1.0/16 && angle_rot < 3.0/16) {
            this->preview_piece.type = TrackPiece::Diagonal;
            this->preview_piece.angle_q = 0;
        } else if(angle_rot >= 3.0/16 && angle_rot < 5.0/16) {
            this->preview_piece.type = TrackPiece::Straight;
            this->preview_piece.angle_q = 1;
        } else if(angle_rot >= 5.0/16 && angle_rot < 7.0/16) {
            this->preview_piece.type = TrackPiece::Diagonal;
            this->preview_piece.angle_q = 1;
        } else {
            this->preview_piece.type = TrackPiece::Straight;
            this->preview_piece.angle_q = 0;
        }
    }

    static const i16 max_piece_elev_d = 1;
    static const f64 max_piece_og_dist = 0.05;

    void TrackingMode::attempt_piece_connection(
        const engine::Window& window, const Renderer& renderer,
        u64 tx, u64 tz, u64 ch_x, u64 ch_z, i16 elev_d, size_t pt_i, i8 angle_q,
        f64& closest_cursor_dist
    ) {
        const auto& p_info = TrackPiece::types().at(pt_i);
        TrackPiece piece = TrackPiece(
            tx % this->world->terrain.tiles_per_chunk(), 
            tz % this->world->terrain.tiles_per_chunk(),
            (TrackPiece::Type) pt_i,
            angle_q, (i16) this->drag_origin->y() + elev_d
        );
        Mat<4> t = piece.build_transform(
            ch_x, ch_z, 
            this->world->terrain.tiles_per_chunk(), 
            this->world->terrain.units_per_tile()
        );
        Vec<3> a = (t * p_info.points[0].with(1.0))
            .swizzle<3>("xyz");
        f64 a_og_dist = (*this->drag_origin - a).len();
        Vec<3> b = (t * p_info.points.back().with(1.0))
            .swizzle<3>("xyz");
        f64 b_og_dist = (*this->drag_origin - b).len();
        Vec<3> o = a_og_dist < b_og_dist? a : b;
        if((o - *this->drag_origin).len() > max_piece_og_dist) { return; }
        Vec<3> c = a_og_dist < b_og_dist? b : a;
        Vec<2> c_ndc = renderer.world_to_ndc(c);
        f64 c_dist = (window.cursor_pos_ndc() - c_ndc).len();
        if(c_dist > closest_cursor_dist) { return; }
        closest_cursor_dist = c_dist;
        this->preview_piece = piece;
        this->preview_ch_x = ch_x;
        this->preview_ch_z = ch_z;
    }

    void TrackingMode::select_piece_connected(
        const engine::Window& window, const Renderer& renderer,
        Vec<3> closest_marker
    ) {
        bool started_dragging = !this->drag_origin.has_value()
            && window.is_down(engine::Button::Left);
        if(started_dragging) { this->drag_origin = closest_marker; }
        bool stopped_dragging = this->drag_origin.has_value()
            && !window.is_down(engine::Button::Left);
        if(stopped_dragging) { this->drag_origin = std::nullopt; }
        if(!this->drag_origin.has_value()) { return; }
        const Terrain& terrain = this->world->terrain;
        u64 origin_tx = (u64) this->drag_origin->x() / terrain.units_per_tile();
        u64 origin_tz = (u64) this->drag_origin->z() / terrain.units_per_tile();
        u64 start_tx = origin_tx >= 1? origin_tx - 1 : origin_tx;
        u64 start_tz = origin_tz >= 1? origin_tz - 1 : origin_tz;
        u64 end_tx = std::min(origin_tx + 1, terrain.width_in_tiles() - 1);
        u64 end_tz = std::min(origin_tz + 1, terrain.height_in_tiles() - 1);
        size_t piece_type_c = TrackPiece::types().size();
        f64 closest_cursor_dist = INFINITY;
        for(u64 tx = start_tx; tx <= end_tx; tx += 1) {
            for(u64 tz = start_tz; tz <= end_tz; tz += 1) {
                u64 ch_x = tx / terrain.tiles_per_chunk();
                u64 ch_z = tz / terrain.tiles_per_chunk();
                i16 min_ed = -max_piece_elev_d;
                i16 max_ed = +max_piece_elev_d;
                for(i16 elev_d = min_ed; elev_d <= max_ed; elev_d += 1) {
                    for(size_t pt_i = 0; pt_i < piece_type_c; pt_i += 1) {
                        if(!TrackPiece::types().at(pt_i).has_ballast) {
                            continue;
                        }
                        for(i8 angle_q = 0; angle_q < 4; angle_q += 1) {
                            this->attempt_piece_connection(
                                window, renderer, tx, tz, ch_x, ch_z, elev_d, 
                                pt_i, angle_q, closest_cursor_dist
                            );
                        }
                    }
                }
            }
        }
    }

    void TrackingMode::determine_piece_valid(u64 tile_x, u64 tile_z) {
        const TrackPiece::TypeInfo& preview_piece_info = TrackPiece::types()
            .at((size_t) this->preview_piece.type);
        Mat<4> piece_instance = this->preview_piece.build_transform(
            this->preview_ch_x, this->preview_ch_z, 
            this->world->terrain.tiles_per_chunk(), 
            this->world->terrain.units_per_tile()
        );
        this->placement_valid = this->world->terrain
            .building_at((i64) tile_x, (i64) tile_z) == nullptr;
        std::vector<const TrackPiece*> existing_pieces;
        this->world->terrain
            .track_pieces_at((i64) tile_x, (i64) tile_z, &existing_pieces);
        for(const TrackPiece* existing_piece: existing_pieces) {
            bool eq = existing_piece->type == this->preview_piece.type
                && existing_piece->x == this->preview_piece.x
                && existing_piece->z == this->preview_piece.z
                && existing_piece->angle_q == this->preview_piece.angle_q;
            this->placement_valid &= !eq;
        }
        for(const Vec<3>& model_point: preview_piece_info.points) {
            Vec<3> point = (piece_instance * model_point.with(1.0))
                .swizzle<3>("xyz");
            const Bridge* on_bridge = this->world->terrain
                .bridge_at((i64) tile_x, (i64) tile_z, point.y());
            if(on_bridge != nullptr) {
                if(preview_piece_info.ballastless.has_value()) {
                    this->preview_piece.type = *preview_piece_info.ballastless;
                }
                bool has_ballast = TrackPiece::types()
                    .at((size_t) this->preview_piece.type).has_ballast;
                this->placement_valid &= !has_ballast;
            }
            f64 elevation = this->world->terrain.elevation_at(point);
            this->placement_valid &= point.y() >= elevation;
            this->placement_valid &= point.y() - elevation <= 1;
        }
    }

    static const u64 track_placement_cost = 100;
    static const u64 track_removal_refund = 50;

    void TrackingMode::update(
        const engine::Window& window, engine::Scene& scene, 
        const Renderer& renderer
    ) {
        (void) scene;
        this->speaker.update();
        // determine the track we we want to place
        auto [tile_x, tile_z] = this->world->terrain.find_selected_terrain_tile(
            window.cursor_pos_ndc(), renderer, Vec<3>(0.5, 0, 0.5)
        );
        std::optional<Vec<3>> closest_marker;
        this->update_track_markers(
            window, renderer, tile_x, tile_z, closest_marker
        );
        if(closest_marker.has_value()) {
            this->select_piece_connected(window, renderer, *closest_marker);
        } else {
            this->select_piece_disconnected(window, renderer, tile_x, tile_z);
        }
        u64 dx = this->preview_ch_x * this->world->terrain.tiles_per_chunk()
            + this->preview_piece.x;
        u64 dz = this->preview_ch_z * this->world->terrain.tiles_per_chunk()
            + this->preview_piece.z;
        this->determine_piece_valid(dx, dz);
        bool place_track = this->permitted
            && this->placement_valid 
            && window.was_released(engine::Button::Left)
            && !this->ui.is_hovered_over()
            && this->world->balance.pay_coins(track_placement_cost, this->toasts);
        if(place_track) {
            Terrain::ChunkData& chunk = this->world->terrain
                .chunk_at(this->preview_ch_x, this->preview_ch_z);
            chunk.track_pieces.push_back(this->preview_piece);
            this->world->terrain.remove_foliage_at((i64) dx, (i64) dz);
            this->world->trains.find_paths(&this->toasts);
            this->speaker.position = Vec<3>(dx + 0.5, 0.0, dz + 0.5)
                * this->world->terrain.units_per_tile()
                + Vec<3>(0, this->preview_piece.elevation, 0);
            this->speaker.play(scene.get(sound::build));
        }
    }

    void TrackingMode::render(
        const engine::Window& window, engine::Scene& scene, 
        Renderer& renderer
    ) {
        (void) window;
        if(!this->permitted) { return; }
        const engine::Texture& wireframe_texture = this->placement_valid
            ? scene.get(ActionMode::wireframe_valid_texture)
            : scene.get(ActionMode::wireframe_error_texture);
        Mat<4> piece_instance = this->preview_piece.build_transform(
            this->preview_ch_x, this->preview_ch_z, 
            this->world->terrain.tiles_per_chunk(), 
            this->world->terrain.units_per_tile()
        );
        const engine::Model::LoadArgs& model = TrackPiece::types()
            .at((size_t) this->preview_piece.type).model;
        renderer.render(
            scene.get(model), 
            std::array { piece_instance }, nullptr, 0.0,
            engine::FaceCulling::Enabled, engine::Rendering::Wireframe,
            engine::DepthTesting::Enabled,
            &wireframe_texture
        );
    }



    static const f64 demolition_refund_factor = 0.25;

    void DemolitionMode::attempt_demolition(engine::Scene& scene) {
        switch(this->selection.type) {
            case Selection::None: return;
            case Selection::Building: {
                const Selection::BuildingSelection& building 
                    = this->selection.value.building;
                const Building::TypeInfo& b_type 
                    = building.selected->get_type_info();
                if(!b_type.destructible) {
                    this->toasts.add_error("toast_indestructible", {});
                    return;
                }
                i64 unemployment = this->world->terrain.compute_unemployment();
                bool enough_people = unemployment >= (i64) b_type.residents;
                if(!enough_people) {
                    this->toasts.add_error("toast_missing_unemployment", {
                        std::to_string(unemployment), 
                        std::to_string(b_type.residents)
                    });
                    return;
                }
                this->speaker.position = tile_bounded_position(
                    building.tile_x, building.tile_z, 
                    building.tile_x + b_type.width, 
                    building.tile_z + b_type.height,
                    this->world->player.character.position,
                    this->world->terrain
                );
                Terrain::ChunkData& chunk = this->world->terrain
                    .chunk_at(building.chunk_x, building.chunk_z);
                if(building.selected->complex.has_value()) {
                    u64 actual_x = building.selected->x
                        + building.chunk_x * this->world->terrain.tiles_per_chunk();
                    u64 actual_z = building.selected->z
                        + building.chunk_z * this->world->terrain.tiles_per_chunk();
                    ComplexId complex_id = *building.selected->complex;
                    Complex& complex = this->world->complexes.get(complex_id);
                    complex.remove_member(actual_x, actual_z);
                    if(complex.member_count() == 0) {
                        this->world->complexes.delete_complex(complex_id);
                    }
                }
                size_t building_idx = building.selected - chunk.buildings.data();
                chunk.buildings.erase(chunk.buildings.begin() + building_idx);
                u64 refunded = (u64) ((f64) b_type.cost * demolition_refund_factor);
                this->world->balance.add_coins(refunded, this->toasts);
                for(i64 ch_o_x = -1; ch_o_x <= 1; ch_o_x += 1) {
                    for(i64 ch_o_z = -1; ch_o_z <= 1; ch_o_z += 1) {
                        i64 ch_x = (i64) building.chunk_x + ch_o_x;
                        i64 ch_z = (i64) building.chunk_z + ch_o_z;
                        bool in_bounds = ch_x >= 0 && ch_z >= 0
                            && (u64) ch_x < this->world->terrain.width_in_chunks()
                            && (u64) ch_z < this->world->terrain.height_in_chunks();
                        if(!in_bounds) { continue; }
                        this->world->terrain
                            .reload_chunk_at((u64) ch_x, (u64) ch_z);
                    }
                }
                this->world->carriages.find_paths(&this->toasts);
                this->selection.type = Selection::None;
                this->speaker.play(scene.get(sound::demolish));
                return;
            }
            case Selection::Bridge: {
                const Bridge* bridge = this->selection.value.bridge;
                const Bridge::TypeInfo& b_type = bridge->get_type_info();
                this->speaker.position = tile_bounded_position(
                    bridge->start_x, bridge->start_z, 
                    bridge->end_x, bridge->end_z,
                    this->world->player.character.position,
                    this->world->terrain
                );
                u64 tiles_per_chunk = this->world->terrain.tiles_per_chunk();
                u64 min_ch_x = bridge->start_x / tiles_per_chunk;
                u64 min_ch_z = bridge->start_z / tiles_per_chunk;
                u64 max_ch_x = bridge->end_x / tiles_per_chunk;
                u64 max_ch_z = bridge->end_z / tiles_per_chunk;
                for(u64 ch_x = min_ch_x; ch_x <= max_ch_x; ch_x += 1) {
                    for(u64 ch_z = min_ch_z; ch_z <= max_ch_z; ch_z += 1) {
                        Terrain::ChunkData& chunk = this->world->terrain
                            .chunk_at(ch_x, ch_z);
                        for(size_t tp_i = 0; tp_i < chunk.track_pieces.size();) {
                            const TrackPiece& tp = chunk.track_pieces[tp_i];
                            u64 tp_x = ch_x * tiles_per_chunk + tp.x;
                            u64 tp_z = ch_z * tiles_per_chunk + tp.z;
                            bool is_affected = tp_x >= bridge->start_x 
                                && tp_x <= bridge->end_x
                                && tp_z >= bridge->start_z 
                                && tp_z <= bridge->end_z
                                && tp.elevation - bridge->floor_y <= 1;
                            if(!is_affected) { 
                                tp_i += 1;
                                continue; 
                            }
                            chunk.track_pieces.erase(
                                chunk.track_pieces.begin() + tp_i
                            );
                            this->world->terrain.reload_chunk_at(ch_x, ch_z);
                        }
                    }
                }
                u64 build_cost = bridge->length() * b_type.cost_per_tile;
                u64 refunded = (u64) ((f64) build_cost * demolition_refund_factor);
                size_t bridge_idx = bridge - this->world->terrain.bridges.data();
                this->world->terrain.bridges.erase(
                    this->world->terrain.bridges.begin() + bridge_idx
                );
                this->world->balance.add_coins(refunded, this->toasts);
                this->world->carriages.find_paths(&this->toasts);
                this->selection.type = Selection::None;
                this->speaker.play(scene.get(sound::demolish));
                return;
            }
            case Selection::TrackPiece: {
                const Selection::TrackPieceSelection& tp_s
                    = this->selection.value.track_piece;
                Terrain::ChunkData& chunk = this->world->terrain
                    .chunk_at(tp_s.chunk_x, tp_s.chunk_z);
                const TrackPiece& removed_piece 
                    = chunk.track_pieces[tp_s.piece_i];
                this->speaker.position 
                    = Vec<3>(tp_s.tile_x + 0.5, 0.0, tp_s.tile_z + 0.5)
                    * this->world->terrain.units_per_tile()
                    + Vec<3>(0, removed_piece.elevation, 0);
                chunk.track_pieces.erase(
                    chunk.track_pieces.begin() + tp_s.piece_i
                );
                this->world->balance
                    .add_coins(track_removal_refund, this->toasts);
                this->world->terrain
                    .reload_chunk_at(tp_s.chunk_x, tp_s.chunk_z);
                this->world->trains.find_paths(&this->toasts);
                this->selection.type = Selection::None;
                this->speaker.play(scene.get(sound::demolish));
                return;
            }
        }
    }

    void DemolitionMode::update(
        const engine::Window& window, engine::Scene& scene, 
        const Renderer& renderer
    ) {
        (void) scene;
        this->speaker.update();
        this->selection.type = Selection::None;
        auto [tile_x, tile_z] = this->world->terrain.find_selected_terrain_tile(
            window.cursor_pos_ndc(), renderer, Vec<3>(0.5, 0, 0.5)
        );
        u64 chunk_x = tile_x / this->world->terrain.tiles_per_chunk();
        u64 chunk_z = tile_z / this->world->terrain.tiles_per_chunk();
        Terrain::ChunkData& hover_chunk 
            = this->world->terrain.chunk_at(chunk_x, chunk_z);
        // check for selected brige
        const Bridge* hover_bridge = this->world->terrain
            .bridge_at((i64) tile_x, (i64) tile_z);
        if(hover_bridge != nullptr) {
            this->selection.type = Selection::Bridge;
            this->selection.value.bridge = hover_bridge;
        }
        // check for track pieces
        u64 rel_ch_x = tile_x % this->world->terrain.tiles_per_chunk();
        u64 rel_ch_z = tile_z % this->world->terrain.tiles_per_chunk();
        f64 closest_track_point_dist = INFINITY;
        for(size_t tp_i = 0; tp_i < hover_chunk.track_pieces.size(); tp_i += 1) {
            const TrackPiece& piece = hover_chunk.track_pieces[tp_i];
            if(piece.x != rel_ch_x || piece.z != rel_ch_z) { continue; }
            const TrackPiece::TypeInfo& piece_info = TrackPiece::types()
                .at((size_t) piece.type);
            Mat<4> t = piece.build_transform(
                chunk_x, chunk_z, 
                this->world->terrain.tiles_per_chunk(), 
                this->world->terrain.units_per_tile()
            );
            for(const Vec<3>& point_model: piece_info.points) {
                Vec<3> point_world = (t * point_model.with(1.0)).swizzle<3>("xyz");
                Vec<2> point_ndc = renderer.world_to_ndc(point_world);
                f64 dist = (window.cursor_pos_ndc() - point_ndc).len();
                if(dist > closest_track_point_dist) { continue; }
                closest_track_point_dist = dist;
                this->selection.type = Selection::TrackPiece;
                this->selection.value.track_piece = { 
                    tile_x, tile_z, chunk_x, chunk_z, tp_i 
                };
            }
        }
        // check for selected building
        u64 hover_building_ch_x, hover_building_ch_z;
        const Building* hover_building = this->world->terrain.building_at(
            (i64) tile_x, (i64) tile_z,
            &hover_building_ch_x, &hover_building_ch_z
        );
        if(hover_building != nullptr) {
            this->selection.type = Selection::Building;
            this->selection.value.building = {
                tile_x, tile_z,
                hover_building_ch_x, hover_building_ch_z,
                hover_building
            };
        }
        // do demolition
        bool attempted = this->selection.type != Selection::None
            && window.was_pressed(engine::Button::Left)
            && !this->ui.is_hovered_over();
        if(attempted) {
            this->attempt_demolition(scene);
        }
    }

    void DemolitionMode::render(
        const engine::Window& window, engine::Scene& scene, 
        Renderer& renderer
    ) {
        const engine::Texture& wireframe_texture 
            = scene.get(ActionMode::wireframe_error_texture);
        switch(this->selection.type) {
            case Selection::None: return;
            case Selection::Building: {
                const Selection::BuildingSelection& building 
                    = this->selection.value.building;
                const Building::TypeInfo& b_type 
                    = building.selected->get_type_info();
                Mat<4> transform = this->world->terrain.building_transform(
                    *building.selected, building.chunk_x, building.chunk_z
                );
                b_type.render_buildings(
                    window, scene, renderer,
                    std::array { transform },
                    engine::Rendering::Wireframe, 
                    &wireframe_texture
                );
                return;
            }
            case Selection::Bridge: {
                const Bridge* bridge = this->selection.value.bridge;
                const Bridge::TypeInfo& b_type = bridge->get_type_info();
                renderer.render(
                    scene.get(b_type.model), 
                    bridge->get_instances(this->world->terrain.units_per_tile()),
                    nullptr, 0.0,
                    engine::FaceCulling::Enabled, engine::Rendering::Wireframe,
                    engine::DepthTesting::Enabled, 
                    &wireframe_texture
                );
                return;
            }
            case Selection::TrackPiece: {
                const Selection::TrackPieceSelection& track_piece_s
                    = this->selection.value.track_piece;
                const Terrain::ChunkData& chunk = this->world->terrain
                    .chunk_at(track_piece_s.chunk_x, track_piece_s.chunk_z);
                const TrackPiece& track_piece 
                    = chunk.track_pieces[track_piece_s.piece_i];
                const engine::Model::LoadArgs& model = TrackPiece::types()
                    .at((size_t) track_piece.type).model;
                Mat<4> instance = track_piece.build_transform(
                    track_piece_s.chunk_x, track_piece_s.chunk_z,
                    this->world->terrain.tiles_per_chunk(),
                    this->world->terrain.units_per_tile()
                );
                renderer.render(
                    scene.get(model), std::array { instance }, nullptr, 0.0,
                    engine::FaceCulling::Enabled, engine::Rendering::Wireframe,
                    engine::DepthTesting::Enabled, 
                    &wireframe_texture
                );
                return;
            }
        }
    }

}