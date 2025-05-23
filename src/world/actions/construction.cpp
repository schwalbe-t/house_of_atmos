
#include "common.hpp"
#include "../../ui_util.hpp"

namespace houseofatmos::world {

    namespace ui = houseofatmos::engine::ui;


    using BuildingVariant = ConstructionMode::BuildingVariant;
    struct BuildingGroup {
        Building::Type type;
        std::vector<BuildingVariant> variants;

        BuildingGroup(Building::Type type, std::vector<BuildingVariant>&& variants) {
            this->type = type;
            this->variants = std::move(variants);
        }
    };
    struct BuildingCategory {
        std::string_view local_name;
        const ui::Background* icon;
        std::vector<BuildingGroup> groups;
    };
    static const std::vector<BuildingCategory> buildable = {
        BuildingCategory(
            "building_category_name_resources",
            &ui_icon::farmland,
            {
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
                BuildingGroup(Building::TreeFarm, {
                    { 
                        { Conversion( 
                            {}, 
                            { { 8, Item::Wood } }, 
                            8.0 
                        ) }
                    }
                })
            }
        ),
        BuildingCategory(
            "building_category_name_production",
            &ui_icon::manufacturing_works,
            {
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
                    },
                    { 
                        { Conversion( 
                            { { 1, Item::Wood } }, 
                            { { 4, Item::Planks } }, 
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
                BuildingGroup(Building::CommissaryWorks, {
                    { 
                        { Conversion( 
                            { { 4, Item::Flour }, { 1, Item::Coal } }, 
                            { { 4, Item::Bread } }, 
                            4.0
                        ) }
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
                            { { 4, Item::Beef }, { 1, Item::Salt }, { 1, Item::Coal } }, 
                            { { 4, Item::Steak } }, 
                            4.0
                        ) },
                        std::nullopt,
                        research::Research::Reward::Steak
                    },
                    { 
                        { Conversion( 
                            { { 8, Item::Milk }, { 1, Item::Salt } }, 
                            { { 4, Item::Cheese } }, 
                            8.0
                        ) },
                        std::nullopt,
                        research::Research::Reward::Cheese
                    },
                    { 
                        { Conversion( 
                            { { 4, Item::Malt }, { 2, Item::Hops }, { 1, Item::Coal } }, 
                            { { 4, Item::Beer } }, 
                            8.0
                        ) },
                        std::nullopt,
                        research::Research::Reward::Beer
                    }
                }),
                BuildingGroup(Building::MetallurgicalWorks, {
                    { 
                        { Conversion( 
                            { { 1, Item::Wood } }, 
                            { { 1, Item::Coal } }, 
                            4.0
                        ) }
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
                            { { 2, Item::IronOre }, { 1, Item::Coal } }, 
                            { { 2, Item::Steel } }, 
                            4.0 
                        ) },
                        std::nullopt,
                        research::Research::Reward::Steel
                    },
                    { 
                        { Conversion( 
                            { { 2, Item::Steel } }, 
                            { { 1, Item::SteelBeams } }, 
                            4.0
                        ) },
                        std::nullopt,
                        research::Research::Reward::SteelBeams
                    },
                    { 
                        { Conversion( 
                            { { 2, Item::CrudeOil }, { 1, Item::Coal } }, 
                            { { 2, Item::Oil } }, 
                            4.0
                        ) },
                        std::nullopt,
                        research::Research::Reward::Oil
                    }
                }),
                BuildingGroup(Building::ManufacturingWorks, {
                    {
                        { Conversion( 
                            { { 4, Item::BrassPlates }, { 4, Item::BrassGears } }, 
                            { { 1, Item::SteamEngines } }, 
                            4.0
                        ) }
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
                        research::Research::Reward::BrassPots
                    },
                    { 
                        { Conversion( 
                            { { 1, Item::BrassPlates }, { 1, Item::BrassRods } }, 
                            { { 1, Item::OilLanterns } }, 
                            4.0
                        ) },
                        std::nullopt,
                        research::Research::Reward::OilLanterns
                    },
                    { 
                        { Conversion( 
                            { { 1, Item::BrassPlates }, { 4, Item::BrassGears } }, 
                            { { 1, Item::Watches } }, 
                            4.0
                        ) },
                        std::nullopt,
                        research::Research::Reward::Watches
                    },
                    { 
                        { Conversion( 
                            { { 2, Item::Planks }, { 4, Item::BrassGears } }, 
                            { { 1, Item::PowerLooms } }, 
                            4.0
                        ) },
                        std::nullopt,
                        research::Research::Reward::PowerLooms
                    },
                    {
                        { Conversion(
                            { { 4, Item::BrassGears }, { 4, Item::Steel } },
                            { { 1, Item::LocomotiveFrames } },
                            8.0
                        ) },
                        std::nullopt,
                        research::Research::Reward::LocomotiveFrames
                    }
                }),
                BuildingGroup(Building::ClothWorks, {
                    { 
                        { Conversion( 
                            { { 1, Item::Cattle } }, 
                            { { 2, Item::Leather } }, 
                            8.0
                        ) }
                    },
                    { 
                        { Conversion( 
                            { { 4, Item::Yarn } }, 
                            { { 1, Item::Fabric } }, 
                            4.0
                        ) },
                        std::nullopt,
                        research::Research::Reward::Fabric
                    },
                    { 
                        { Conversion( 
                            { { 2, Item::Fabric } }, 
                            { { 1, Item::Clothing } }, 
                            4.0
                        ) },
                        std::nullopt,
                        research::Research::Reward::Clothing
                    }
                })
            }
        ),
        BuildingCategory(
            "building_category_name_logistics",
            &ui_icon::sail_boat,
            {
                BuildingGroup(Building::Stable, {}),
                BuildingGroup(Building::TrainDepot, {}),
                BuildingGroup(Building::ShipYard, {}),
                BuildingGroup(Building::Storage, {
                    {} 
                })
            }
        )
    };

    struct SelectionContext {
        const engine::Localization* local; 
        const research::Research* research;
        ui::Manager* ui;
        ui::Element* info;
        ui::Element* selection;
        Building::Type* s_type;
        const ConstructionMode::BuildingVariant** s_variant;
    };

    static void show_category_selector(SelectionContext ctx);
    static void show_group_selector(
        SelectionContext ctx, const BuildingCategory* category
    );

    static const size_t max_column_variants = 15;

    static void show_variant_selector(
        SelectionContext ctx, 
        const BuildingCategory* category, const BuildingGroup* group
    ) {
        ui::Element selector 
            = ui_util::create_selection_container(
                ctx.local->text("ui_product_selection")
            )
            .with_pos(
                ui::horiz::in_window_fract(0.95), ui::vert::in_window_fract(0.5)
            )
            .as_movable();
        ui::Element container = ui::Element()
            .with_list_dir(ui::Direction::Horizontal)
            .as_movable();
        size_t column_count 
            = (group->variants.size() / max_column_variants) + 1;
        for(size_t column_i = 0; column_i < column_count; column_i += 1) {
            ui::Element column = ui::Element()
                .with_list_dir(ui::Direction::Vertical)
                .as_movable();
            size_t first_var_i = column_i * max_column_variants;
            size_t end_var_i = std::min(
                first_var_i + max_column_variants, group->variants.size()
            );
            for(size_t var_i = first_var_i; var_i < end_var_i; var_i += 1) {
                const auto& variant = group->variants[var_i];
                bool is_unlocked = !variant.req_reward.has_value()
                    || ctx.research->is_unlocked(*variant.req_reward);
                if(!is_unlocked) { continue; }
                if(variant.conversions.size() == 0) { continue; }
                if(variant.conversions.at(0).outputs.size() == 0) { continue; }
                const Item::TypeInfo& result = Item::types().at(
                    (size_t) variant.conversions.at(0).outputs[0].item
                );
                column.children.push_back(ui_util::create_selection_item(
                    result.icon, ctx.local->text(result.local_name), false,
                    [ctx, category, type = group->type, variant = &variant]() {
                        *ctx.s_type = type;
                        *ctx.s_variant = variant;
                        *ctx.info = TerrainMap::display_building_info(
                            type, variant->conversions, std::nullopt, *ctx.local
                        );
                        show_group_selector(ctx, category);
                    }
                ));
            }
            container.children.push_back(std::move(column));
        }
        selector.children.push_back(std::move(container));
        selector.children.push_back(ui_util::create_button(
            ctx.local->text("ui_back"),
            [ctx, category]() {
                show_group_selector(ctx, category);
            }
        ));
        *ctx.selection = std::move(selector);
    }   

    static void show_group_selector(
        SelectionContext ctx, const BuildingCategory* category
    ) {
        ui::Element selector 
            = ui_util::create_selection_container(
                ctx.local->text("ui_building_selection")
            )
            .with_pos(
                ui::horiz::in_window_fract(0.95), ui::vert::in_window_fract(0.5)
            )
            .as_movable();
        for(const BuildingGroup& group: category->groups) {
            const Building::TypeInfo& type = Building::types()
                .at((size_t) group.type);
            std::vector<size_t> unlocked_v;
            size_t locked_var_c = group.variants.size();
            for(size_t var_i = 0; var_i < locked_var_c; var_i += 1) {
                const ConstructionMode::BuildingVariant& var
                    = group.variants[var_i];
                bool is_unlocked = !var.req_reward.has_value()
                    || ctx.research->is_unlocked(*var.req_reward);
                if(is_unlocked) { unlocked_v.push_back(var_i); }
            }
            bool not_unlocked = group.variants.size() >= 1
                && unlocked_v.size() == 0;
            if(not_unlocked) { continue; }
            selector.children.push_back(ui_util::create_selection_item(
                type.icon, ctx.local->text(type.local_name), 
                *ctx.s_type == group.type,
                [ctx, category, group = &group, v = std::move(unlocked_v)]() {
                    if(group->variants.size() == 0) {
                        *ctx.s_type = group->type;
                        *ctx.s_variant = nullptr;
                        *ctx.info = TerrainMap::display_building_info(
                            group->type, std::span<Conversion>(), 
                            std::nullopt, *ctx.local
                        );
                        show_group_selector(ctx, category);
                        return;
                    }
                    if(v.size() == 1) {
                        *ctx.s_type = group->type;
                        *ctx.s_variant = &group->variants[v[0]];
                        *ctx.info = TerrainMap::display_building_info(
                            group->type, (**ctx.s_variant).conversions, 
                            std::nullopt, *ctx.local
                        );
                        show_group_selector(ctx, category);
                        return;
                    }
                    show_variant_selector(ctx, category, group);
                }
            ));
        }
        selector.children.push_back(ui_util::create_button(
            ctx.local->text("ui_back"),
            [ctx]() { show_category_selector(ctx); }
        ));
        *ctx.selection = std::move(selector);
    }

    static void show_category_selector(SelectionContext ctx) {
        ui::Element selector 
            = ui_util::create_selection_container(
                ctx.local->text("ui_building_selection")
            )
            .with_pos(
                ui::horiz::in_window_fract(0.95), ui::vert::in_window_fract(0.5)
            )
            .as_movable();
        for(const BuildingCategory& category: buildable) {
            selector.children.push_back(ui_util::create_selection_item(
                category.icon, ctx.local->text(category.local_name), false,
                [ctx, category = &category]() {
                    show_group_selector(ctx, category);
                }
            ));
        }
        *ctx.selection = std::move(selector);
    }

    ConstructionMode::ConstructionMode(
        ActionContext ctx
    ): ActionMode(ctx) {
        this->selected_x = 0;
        this->selected_z = 0;
        this->selected_type 
            = std::make_unique<Building::Type>(Building::Storage);
        this->selected_variant = std::make_unique<const BuildingVariant*>();
        this->placement_valid = false;
    }

    void ConstructionMode::init_ui() {
        this->ui.root.children.push_back(ui::Element().as_phantom().as_movable());
        ui::Element* selector = &this->ui.root.children.back();
        this->ui.root.children.push_back(ui::Element().as_phantom().as_movable());
        ui::Element* selected = &this->ui.root.children.back();
        *selected = TerrainMap::display_building_info(
            *this->selected_type,
            *this->selected_variant != nullptr
                ? std::span((**this->selected_variant).conversions)
                : std::span<Conversion>(), 
            std::nullopt, local
        );
        auto ctx = SelectionContext(
            &local, &this->world->research,
            &this->ui, selected, selector,
            &*this->selected_type, &*this->selected_variant
        );
        show_category_selector(ctx);
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
            && building_variant->req_resource.has_value();
        if(!requires_resource) { return true; }
        for(i64 x = tile_x; x < tile_x + (i64) building_type.width; x += 1) {
            for(i64 z = tile_z; z < tile_z +(i64)  building_type.height; z += 1) {
                const Resource* resource = terrain.resource_at(x, z);
                bool is_present = resource != nullptr
                    && resource->type == *building_variant->req_resource;
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
            bool has_resource = required_resource_present(
                this->world->terrain, tile_x, tile_z,
                type_info, *this->selected_variant
            );
            bool allowed = has_resource
                && this->world->balance.pay_coins(type_info.cost, this->toasts);
            if(allowed) {
                place_building(
                    tile_x, tile_z, this->world->terrain, this->world->complexes,
                    *this->selected_type, type_info, *this->selected_variant
                );
                this->world->carriages.reset(&this->toasts);
                this->world->populations.reset(
                    this->world->terrain, &this->toasts
                );
                this->speaker.position = tile_bounded_position(
                    tile_x, tile_z, 
                    tile_x + type_info.width, tile_z + type_info.height,
                    this->world->player.character.position,
                    this->world->terrain
                );
                this->speaker.play(scene.get(sound::build));
            } else if(!has_resource) {
                Resource::Type r_resource = *(**this->selected_variant)
                    .req_resource;
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
            &wireframe_texture
        );
    }
    
}