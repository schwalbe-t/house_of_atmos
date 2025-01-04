
#include "actionmode.hpp"
#include <iostream>

namespace houseofatmos::outside {

    void ActionMode::choose_current(
        const engine::Window& window,
        Terrain& terrain, ComplexBank& complexes, const Player& player,
        std::unique_ptr<ActionMode>& current
    ) {
        for(size_t type_i = 0; type_i < ActionMode::keys.size(); type_i += 1) {
            engine::Key key = ActionMode::keys.at(type_i);
            ActionMode::Type type = (ActionMode::Type) type_i;
            if(!window.was_pressed(key)) { continue; }
            if(current->get_type() == type) {
                current = std::make_unique<DefaultMode>(terrain, complexes); 
                break;
            }
            switch(type) {
                case Default: 
                    current = std::make_unique<DefaultMode>(terrain, complexes); 
                    break;
                case Terraform: 
                    current = std::make_unique<TerraformMode>(terrain); 
                    break;
                case Construction: 
                    current = std::make_unique<ConstructionMode>(
                        terrain, complexes, player
                    );
                    break;
                case Demolition: 
                    current = std::make_unique<DemolitionMode>(
                        terrain, complexes
                    ); 
                    break;
                case Pathing: 
                    current = std::make_unique<PathingMode>(terrain); 
                    break;
                default: engine::warning(
                    "Unhandled 'ActionMode::Type' in 'ActionMode::choose_current'"
                );
            }
            break;
        }
    }



    static std::string get_item_name(Item item) {
        switch(item) {
            case Item::Barley: return "Barley";
            case Item::Malt: return "Malt";
            case Item::Beer: return "Beer";
            case Item::Wheat: return "Wheat";
            case Item::Flour: return "Flour";
            case Item::Bread: return "Bread";
            case Item::Hematite: return "Hematite";
            case Item::Coal: return "Coal";
            case Item::Steel: return "Steel";
            case Item::Armor: return "Armor";
            case Item::Tools: return "Tool(s)";
            case Item::Coins: return "Coin(s)";
            default: engine::error("Unhandled 'Item' in 'get_item_name'");
        }
    }

    static void print_complex_info(ComplexId id, const Complex& complex) {
        std::string output;
        output += "===== Complex #" + std::to_string(id.index) + " =====";
        std::unordered_map<Item, f64> throughput = complex.compute_throughput();
        output += "\n     inputs [/s]: ";
        bool had_input = false;
        for(const auto& [item, freq]: throughput) {
            if(freq >= 0.0) { continue; }
            if(had_input) { output += ", "; }
            output += std::to_string(fabs(freq)) + " " + get_item_name(item);
            had_input = true;
        }
        if(!had_input) { output += "<none>"; }
        output += "\n    outputs [/s]: ";
        bool had_output = false;
        for(const auto& [item, freq]: throughput) {
            if(freq <= 0.0) { continue; }
            if(had_output) { output += ", "; }
            output += std::to_string(freq) + " " + get_item_name(item);
            had_output = true;
        }
        output += "\n         storage: ";
        if(!had_output) { output += "<none>"; }
        bool had_stored = false;
        for(const auto& [item, count]: complex.stored_items()) {
            if(count == 0) { continue; }
            if(had_stored) { output += ", "; }
            output += std::to_string(count) + " " + get_item_name(item);
            had_stored = true;
        }
        if(!had_stored) { output += "<none>"; }
        engine::info(output);
    }

    static std::string get_building_name(Building::Type type) {
        switch(type) {
            case Building::Farmland: return "Farmland";
            case Building::Mineshaft: return "Mineshaft";
            case Building::Windmill: return "Windmill";
            case Building::Factory: return "Factory";
            case Building::House: return "House";
            case Building::Plaza: return "Plaza";
            default: engine::error(
                "Unhandled 'Building::Type' in 'get_building_name'"
            );
        }
    }

    static std::string display_conversion(const Conversion& conversion) {
        std::string output;
        output += "[/" + std::to_string(conversion.period) + "s] ";
        bool had_input = false;
        for(const auto& [count, item]: conversion.inputs) {
            if(had_input) { output += ", "; }
            output += std::to_string(count) + " " + get_item_name(item);
            had_input = true;
        }
        if(had_input) { output += ' '; }
        output += "-> ";
        bool had_output = false;
        for(const auto& [count, item]: conversion.outputs) {
            if(had_output) { output += ", "; }
            output += std::to_string(count) + " " + get_item_name(item);
            had_output = true;
        }
        return output;
    }

    static void print_building_info(
        Building& building, u64 chunk_x, u64 chunk_z, const Complex* complex,
        const Terrain& terrain
    ) {
        Building::TypeInfo type = building.get_type_info();
        u64 tile_x = building.x + chunk_x * terrain.tiles_per_chunk();
        u64 tile_z = building.z + chunk_z * terrain.tiles_per_chunk();
        std::string output;
        output += "===== " + get_building_name(building.type) + " =====";
        if(complex != nullptr) {
            const Complex::Member& member = complex->member_at(tile_x, tile_z);
            output += "\n    production: ";
            bool had_conversion = false;
            for(const Conversion& conversion: member.conversions) {
                output += "\n        " + display_conversion(conversion); 
                had_conversion = true;
            }
            if(!had_conversion) { output += "\n        <none>"; }
        }
        if(type.residents != 0) {
            output += "\n     residents: " + std::to_string(type.residents);
        }
        if(type.workers != 0) {
            output += "\n       workers: " + std::to_string(type.workers);
        }
        engine::info(output);
    }

    void DefaultMode::update(
        const engine::Window& window, engine::Scene& scene, 
        const Renderer& renderer, Balance& balance
    ) {
        (void) scene;
        (void) renderer;
        (void) balance;
        if(!window.was_pressed(engine::Button::Left)) { return; }
        auto [s_tile_x, s_tile_z] = this->terrain.find_selected_terrain_tile(
            window.cursor_pos_ndc(), renderer.compute_view_proj(),
            Vec<3>(0.5, 0, 0.5)
        );
        u64 s_chunk_x, s_chunk_z;
        Building* s_building = this->terrain.building_at(
            (i64) s_tile_x, (i64) s_tile_z, &s_chunk_x, &s_chunk_z
        );
        if(s_building) {
            u64 tile_x = s_building->x
                + s_chunk_x * this->terrain.tiles_per_chunk();
            u64 tile_z = s_building->z
                + s_chunk_z * this->terrain.tiles_per_chunk();
            std::optional<ComplexId> closest = this->complexes
                .closest_to(tile_x, tile_z);
            if(closest.has_value()) {
                const Complex& complex = this->complexes.get(*closest);
                if(complex.has_member_at(tile_x, tile_z)) {
                    print_building_info(
                        *s_building, s_chunk_x, s_chunk_z, &complex,
                        this->terrain
                    );
                    print_complex_info(*closest, complex);
                    this->selected.type = Selection::Complex;
                    this->selected.value.complex = *closest;
                    return;
                }
            }
            print_building_info(
                *s_building, s_chunk_x, s_chunk_z, nullptr, this->terrain
            );
        }
        this->selected.type = Selection::None;
    }

    void DefaultMode::render(
        const engine::Window& window, engine::Scene& scene, 
        const Renderer& renderer
    ) {
        switch(this->selected.type) {
            case Selection::None: break;
            case Selection::Complex: {
                const Complex& complex = this->complexes
                    .get(this->selected.value.complex);
                std::unordered_map<Building::Type, std::vector<Mat<4>>> inst;
                for(const auto& member: complex.get_members()) {
                    const auto& [tile_x, tile_z] = member.first;
                    u64 chunk_x, chunk_z;
                    Building* building = this->terrain.building_at(
                        (i64) tile_x, (i64) tile_z, &chunk_x, &chunk_z
                    );
                    if(building == nullptr) {
                        engine::error(
                            "Complex refers to a building that no longer exists"
                        );
                    }
                    Mat<4> transform = this->terrain
                        .building_transform(*building, chunk_x, chunk_z);
                    inst[building->type].push_back(transform);
                }
                const engine::Texture& wireframe_texture = scene
                    .get<engine::Texture>(ActionMode::wireframe_info_texture);
                for(const auto& [type, instances]: inst) {
                    Building::TypeInfo type_info = Building::types
                        .at((size_t) type);
                    type_info.render_buildings(
                        window, scene, renderer,
                        instances, true, &wireframe_texture
                    );
                }
            } break;
        }
    }



    // <cost> = (100 + 5 ^ <minimum elevation difference>) coins
    static u64 compute_terrain_modification_cost(
        u64 tile_x, u64 tile_z, i64 elevation, const Terrain& terrain
    ) {
        u64 min_elev_diff = UINT64_MAX;
        for(i64 offset_x = -1; offset_x <= 1; offset_x += 1) {
            for(i64 offset_z = -1; offset_z <= 1; offset_z += 1) {
                if(offset_x == 0 && offset_z == 0) { continue; }
                if((i64) tile_x + offset_x < 0) { continue; }
                if((i64) tile_z + offset_z < 0) { continue; }
                i64 elev = terrain.elevation_at(
                    (u64) ((i64) tile_x + offset_x), 
                    (u64) ((i64) tile_z + offset_z)
                );
                u64 diff = (u64) abs(elevation - elev);
                min_elev_diff = std::min(min_elev_diff, diff);
            }
        }
        return 100 + (u64) pow(5, (min_elev_diff + 1));
    }

    static bool modified_terrain_occupied(
        i64 tile_x, i64 tile_z, Terrain& terrain
    ) {
        return terrain.building_at(tile_x - 1, tile_z - 1) != nullptr
            || terrain.building_at(tile_x - 1, tile_z) != nullptr
            || terrain.building_at(tile_x, tile_z - 1) != nullptr
            || terrain.building_at(tile_x, tile_z) != nullptr;
    }

    static void modify_terrain_height(
        u64 tile_x, u64 tile_z, Terrain& terrain, i16 modification
    ) {
        terrain.elevation_at(tile_x, tile_z) += modification;
        for(i64 offset_x = -1; offset_x <= 0; offset_x += 1) {
            for(i64 offset_z = -1; offset_z <= 0; offset_z += 1) {
                terrain.remove_foliage_at(
                    (i64) tile_x + offset_x, (i64) tile_z + offset_z
                );
            }
        }
    }

    void TerraformMode::update(
        const engine::Window& window, engine::Scene& scene, 
        const Renderer& renderer, Balance& balance
    ) {
        (void) scene;
        (void) renderer;
        auto [tile_x, tile_z] = this->terrain.find_selected_terrain_tile(
            window.cursor_pos_ndc(), renderer.compute_view_proj(),
            Vec<3>(0, 0, 0)
        );
        this->selected_x = tile_x;
        this->selected_z = tile_z;
        this->modification_valid = !modified_terrain_occupied(
            (i64) tile_x, (i64) tile_z, this->terrain
        );
        bool modified_terrain = window.was_pressed(engine::Button::Left)
            || window.was_pressed(engine::Button::Right);
        if(modified_terrain) {
            i16 elevation = this->terrain.elevation_at(tile_x, tile_z);
            i16 modification = 0;
            if(window.was_pressed(engine::Button::Left)) {
                modification += 1;
            }
            if(window.was_pressed(engine::Button::Right)) {
                modification -= 1;
            }
            u64 cost = compute_terrain_modification_cost(
                tile_x, tile_z, elevation, this->terrain
            );
            if(this->modification_valid && balance.pay_coins(cost)) {
                modify_terrain_height(
                    tile_x, tile_z, this->terrain, modification
                );
            }
        }
    }

    void TerraformMode::render(
        const engine::Window& window, engine::Scene& scene, 
        const Renderer& renderer
    ) {
        (void) window;
        i16& elevation = this->terrain.elevation_at(
            this->selected_x, this->selected_z
        );
        u64 chunk_x = this->selected_x / this->terrain.tiles_per_chunk();
        u64 chunk_z = this->selected_z / this->terrain.tiles_per_chunk();
        Vec<3> offset = Vec<3>(chunk_x, 0, chunk_z) 
            * this->terrain.tiles_per_chunk()
            * this->terrain.units_per_tile();
        Mat<4> transform = Mat<4>::translate(offset);
        const engine::Texture& wireframe_add_texture = this->modification_valid
            ? scene.get<engine::Texture>(ActionMode::wireframe_add_texture)
            : scene.get<engine::Texture>(ActionMode::wireframe_error_texture);
        elevation += 1;
        engine::Mesh add_geometry = this->terrain
            .build_chunk_geometry(chunk_x, chunk_z);
        renderer.render(
            add_geometry, wireframe_add_texture, 
            Mat<4>(), std::array { transform }, true
        );
        elevation -= 1;
        const engine::Texture& wireframe_sub_texture = scene
            .get<engine::Texture>(ActionMode::wireframe_sub_texture);
        elevation -= 1;
        engine::Mesh sub_geometry = this->terrain
            .build_chunk_geometry(chunk_x, chunk_z);
        renderer.render(
            sub_geometry, wireframe_sub_texture, 
            Mat<4>(), std::array { transform }, true
        );
        elevation += 1;     
    }



    static std::string get_text_input() {
        std::cout << "> ";
        std::string input;
        std::getline(std::cin, input);
        return input;
    }

    static void choose_building_type(
        const engine::Window& window, 
        Building::Type& type,
        std::vector<Conversion>& conversions
    ) {
        if(window.was_pressed(engine::Key::Enter)) {
            engine::info(
                "Enter a building type to build "
                "(one of: 'house', 'farmland', 'mineshaft', 'windmill', 'factory', 'stable')"
            );
            std::string type_name = get_text_input();
            if(type_name == "house") {
                type = Building::House;
                conversions.clear();
                return;
            }
            if(type_name == "farmland") {
                engine::info("Enter what the farmland should produce (one of: 'wheat', 'barley')");
                std::string produce_name = get_text_input();
                if(produce_name == "wheat") {
                    type = Building::Farmland;
                    conversions = { Conversion(
                        {}, 
                        { { 10, Item::Wheat } }, 
                        10.0
                    ) };
                    engine::info("Selected farmland (-> 40 Wheat / 10 s)");
                    return;
                }
                if(produce_name == "barley") {
                    type = Building::Farmland;
                    conversions = { Conversion(
                        {}, 
                        { { 10, Item::Barley } }, 
                        10.0
                    ) };
                    engine::info("Selected farmland (-> 40 Barley / 10 s)");
                    return;
                }
            }
            if(type_name == "mineshaft") {
                engine::info("Enter what the mineshaft should produce (one of: 'hematite', 'coal')");
                std::string produce_name = get_text_input();
                if(produce_name == "hematite") {
                    type = Building::Mineshaft;
                    conversions = { Conversion(
                        {}, 
                        { { 1, Item::Hematite } }, 
                        1.0
                    ) };
                    engine::info("Selected mineshaft (-> 1 Hematite / 1 s)");
                    return;
                }
                if(produce_name == "coal") {
                    type = Building::Mineshaft;
                    conversions = { Conversion(
                        {}, 
                        { { 1, Item::Coal } }, 
                        1.0
                    ) };
                    engine::info("Selected coal (-> 1 Coal / 1 s)");
                    return;
                }
            }
            if(type_name == "windmill") {
                engine::info("Enter what the windmill should produce (one of: 'malt', 'flour')");
                std::string produce_name = get_text_input();
                if(produce_name == "malt") {
                    type = Building::Windmill;
                    conversions = { Conversion(
                        { { 4, Item::Barley } }, 
                        { { 1, Item::Malt } },
                        1.0
                    ) };
                    engine::info("Selected windmill (4 Barley -> 1 Malt / 1 s)");
                    return;
                }
                if(produce_name == "flour") {
                    type = Building::Windmill;
                    conversions = { Conversion(
                        { { 4, Item::Wheat } }, 
                        { { 1, Item::Flour } }, 
                        1.0
                    ) };
                    engine::info("Selected windmill (4 Wheat -> 1 Flour / 1 s)");
                    return;
                }
            }
            if(type_name == "factory") {
                type = Building::House;
                engine::info(
                    "Enter what the factory should produce "
                    "(one of: 'beer', 'bread', 'steel', 'armor', 'tools')"    
                );
                std::string produce_name = get_text_input();
                if(produce_name == "beer") {
                    type = Building::Factory;
                    conversions = { Conversion(
                        { { 1, Item::Malt } }, 
                        { { 4, Item::Beer } }, 
                        4.0
                    ) };
                    engine::info("Selected factory (1 Malt -> 4 Beer / 4 s)");
                    return;
                }
                if(produce_name == "bread") {
                    type = Building::Factory;
                    conversions = { Conversion(
                        { { 1, Item::Flour } }, 
                        { { 2, Item::Bread } }, 
                        2.0
                    ) };
                    engine::info("Selected factory (1 Flour -> 2 Bread / 2 s)");
                    return;
                }
                if(produce_name == "steel") {
                    type = Building::Factory;
                    conversions = { Conversion(
                        { { 2, Item::Hematite }, { 1, Item::Coal } }, 
                        { { 2, Item::Steel } }, 
                        5.0
                    ) };
                    engine::info("Selected factory (2 Hematite + 1 Coal -> 2 Steel / 5 s)");
                    return;
                }
                if(produce_name == "armor") {
                    type = Building::Factory;
                    conversions = { Conversion(
                        { { 3, Item::Steel } }, 
                        { { 1, Item::Armor } }, 
                        10.0
                    ) };
                    engine::info("Selected factory (3 Steel -> 1 Armor / 10 s)");
                    return;
                }
                if(produce_name == "tools") {
                    type = Building::Factory;
                    conversions = { Conversion(
                        { { 2, Item::Steel } }, 
                        { { 1, Item::Tools } }, 
                        5.0
                    ) };
                    engine::info("Selected factory (2 Steel -> 1 Tools / 5 s)");
                    return;
                }
            }
            if(type_name == "stable") {
                type = Building::Stable;
                conversions.clear();
                return;
            }
            engine::info("Invalid input, cancelled. Press enter to try again.");
        }
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
        i64 start_x = (i64) tile_x - (i64) type_info.width / 2;
        i64 end_x = (i64) tile_x + (i64) ceil(type_info.width / 2.0);
        i64 start_z = (i64) tile_z - (i64) type_info.height / 2;
        i64 end_z = (i64) tile_z + (i64) ceil(type_info.height / 2.0);
        for(i64 u_tile_x = start_x; u_tile_x < end_x; u_tile_x += 1) {
            for(i64 u_tile_z = start_z; u_tile_z < end_z; u_tile_z += 1) {
                terrain.remove_foliage_at(u_tile_x, u_tile_z);
            }
        }
    }

    void ConstructionMode::update(
        const engine::Window& window, engine::Scene& scene, 
        const Renderer& renderer, Balance& balance
    ) {
        (void) scene;
        (void) renderer;
        choose_building_type(window, this->selected_type, this->selected_conversion);
        const Building::TypeInfo& type_info = Building::types
            .at((size_t) this->selected_type);
        auto [tile_x, tile_z] = this->terrain.find_selected_terrain_tile(
            window.cursor_pos_ndc(), renderer.compute_view_proj(),
            Vec<3>(type_info.offset_x, 0, type_info.offset_z)
        );
        this->selected_x = tile_x;
        this->selected_z = tile_z;
        this->placement_valid = this->terrain.valid_building_location(
            (i64) tile_x, (i64) tile_z, this->player.position, type_info
        );
        if(this->placement_valid && window.was_pressed(engine::Button::Left)) {
            i64 unemployment = this->terrain.compute_unemployment();
            bool allowed = unemployment >= (i64) type_info.workers
                && balance.pay_coins(type_info.cost);
            if(allowed) {
                place_building(
                    tile_x, tile_z, this->terrain, this->complexes,
                    this->selected_type, type_info, this->selected_conversion
                );
            } else if(unemployment < (i64) type_info.workers) {
                engine::info("Not enough unemployed workers available! ("
                    + std::to_string(unemployment) + "/"
                    + std::to_string(type_info.workers) + ")"
                );
            }
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
            this->selected_type, 
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
        const Renderer& renderer, Balance& balance
    ) {
        (void) scene;
        auto [tile_x, tile_z] = this->terrain.find_selected_terrain_tile(
            window.cursor_pos_ndc(), renderer.compute_view_proj(),
            Vec<3>(0.5, 0, 0.5)
        );
        this->selected_tile_x = tile_x;
        this->selected_tile_z = tile_z;
        this->selected = terrain.building_at(
            (i64) tile_x, (i64) tile_z, 
            &this->selected_chunk_x, &this->selected_chunk_z
        );
        if(this->selected != nullptr && window.was_pressed(engine::Button::Left)) {
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
                balance.coins += refunded;
                this->terrain.reload_chunk_at(
                    this->selected_chunk_x, this->selected_chunk_z
                );
                engine::info("Refunded " + std::to_string(refunded) + " coins "
                    "for building demolition (now " + std::to_string(balance.coins) + ")"
                );
            } else {
                engine::info("Not enough unemployed workers available! ("
                    + std::to_string(unemployment) + "/"
                    + std::to_string(type_info.residents) + ") "
                );
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
        const Renderer& renderer, Balance& balance
    ) {
        (void) scene;
        auto [tile_x, tile_z] = this->terrain.find_selected_terrain_tile(
            window.cursor_pos_ndc(), renderer.compute_view_proj(),
            Vec<3>(0.5, 0, 0.5)
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
            && valid_path_location(tile_x, tile_z, this->terrain)
            && balance.pay_coins(path_placement_cost);
        if(place_path) {
            chunk.set_path_at(rel_x, rel_z, true);
            this->terrain.remove_foliage_at((i64) tile_x, (i64) tile_z);
            this->terrain.reload_chunk_at(chunk_x, chunk_z);
        } else if(has_path && window.was_pressed(engine::Button::Right)) {
            chunk.set_path_at(rel_x, rel_z, false);
            this->terrain.reload_chunk_at(chunk_x, chunk_z);
            balance.coins += path_removal_refund;
            engine::info("Refunded " + std::to_string(path_removal_refund) + " coins "
                "for path removal (now " + std::to_string(balance.coins) + ")"
            );
        }
    }

}