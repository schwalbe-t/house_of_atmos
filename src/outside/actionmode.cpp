
#include "actionmode.hpp"
#include <iostream>

namespace houseofatmos::outside {

    namespace ui = houseofatmos::engine::ui;



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
        if(!had_output) { output += "<none>"; }
        output += "\n         storage: ";
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
            case Building::Stable: return "Stable";
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
        if(building.type == Building::Stable) {
            output += "\n    (press enter to manage carriages.)";
        }
        engine::info(output);
    }

    static std::string get_text_input() {
        std::cout << "> ";
        std::string input;
        std::getline(std::cin, input);
        return input;
    }

    static std::string display_scheduled_stop(
        const Carriage& carriage, size_t tgt_i
    ) {
        std::string output;
        output += "[" + std::to_string(tgt_i) + "] ";
        const Carriage::Target& target = carriage.targets[tgt_i];
        output += "complex #" + std::to_string(target.complex.index) + ", ";
        switch(target.action) {
            case Carriage::LoadFixed: case Carriage::LoadPercentage:
                output += "load "; break;
            case Carriage::PutFixed: case Carriage::PutPercentage:
                output += "unload "; break;
        }
        switch(target.action) {
            case Carriage::LoadFixed: case Carriage::PutFixed:
                output += std::to_string(target.amount.fixed) + " "; break;
            case Carriage::LoadPercentage: case Carriage::PutPercentage:
                output += std::to_string(target.amount.percentage * 100) + "% of "; break;
        }
        output += get_item_name(target.item);
        return output;
    }

    static void print_carriage_info(
        const CarriageManager& carriages, size_t carr_i
    ) {
        const Carriage& carriage = carriages.carriages[carr_i];
        std::string output;
        output += "===== Carriage #" + std::to_string(carr_i) + " =====";
        output += "\n       schedule:";
        for(size_t tgt_i = 0; tgt_i < carriage.targets.size(); tgt_i += 1) {
            output += "\n        " + display_scheduled_stop(carriage, tgt_i);
        }
        output += "\n    destination: ";
        if(carriage.is_lost()) {
             output += "<unable to find path!> ";
        }
        if(carriage.has_path()) {
            output += "\n        " 
                + display_scheduled_stop(carriage, *carriage.target_i());
        } else {
            output += "<none>";
        }
        output += "\n        storage: ";
        bool had_stored = false;
        for(const auto& [item, count]: carriage.stored_items()) {
            if(count == 0) { continue; }
            if(had_stored) { output += ", "; }
            output += std::to_string(count) + " " + get_item_name(item);
            had_stored = true;
        }
        if(!had_stored) { output += "<none>"; }
        engine::info(output);
    }

    static void manage_carriage(
        CarriageManager& carriages, unsigned long long int carr_i,
        const ComplexBank& complexes
    ) {
        print_carriage_info(carriages, (size_t) carr_i);
        Carriage& carriage = carriages.carriages[carr_i];
        engine::info("Enter what to do with the carriage ('remove', 'manage'):");
        std::string carr_action = get_text_input();
        if(carr_action == "remove") {
            carriages.carriages.erase(carriages.carriages.begin() + carr_i);
            engine::info("Removed carriage with index ["
                + std::to_string(carr_i) + "]. (Please note that this may "
                "have changed the indices of other carriages.)"
            );
            return;
        }
        if(carr_action != "manage") {
            engine::info("Invalid input, cancelled. Press enter to try again.");
            return;
        }
        engine::info(
            "Enter the position of a stop to manage, "
            "or enter 'add' to add a new one:"
        );
        std::string target_i_str = get_text_input();
        if(target_i_str == "add") {
            Carriage::Target target;
            engine::info("Enter the index of the building complex to stop at:");
            std::string complex_i_str = get_text_input();
            unsigned long long int complex_i = ULLONG_MAX;
            sscanf(complex_i_str.c_str(), "%llu", &complex_i);
            if(complexes.get_arbitrary(complex_i) == nullptr) {
                engine::info("Invalid input, cancelled. Press enter to try again.");
                return;
            }
            target.complex = (ComplexId) { (u32) complex_i };
            engine::info("Enter whether to load or unload items ('load', 'unload'):");
            std::string action_str = get_text_input();
            if(action_str != "load" && action_str != "unload") {
                engine::info("Invalid input, cancelled. Press enter to try again.");
                return;
            }
            engine::info(
                "Enter the item to transfer ('barley', 'malt', 'beer', "
                "'wheat', 'flour', 'bread', 'hematite', 'coal', 'steel', "
                "'armor', 'tools'):"
            );
            std::string item_str = get_text_input();
            if(item_str == "barley") { target.item = Item::Barley; }
            else if(item_str == "malt") { target.item = Item::Malt; }
            else if(item_str == "beer") { target.item = Item::Beer; }
            else if(item_str == "wheat") { target.item = Item::Wheat; }
            else if(item_str == "flour") { target.item = Item::Flour; }
            else if(item_str == "bread") { target.item = Item::Bread; }
            else if(item_str == "hematite") { target.item = Item::Hematite; }
            else if(item_str == "coal") { target.item = Item::Coal; }
            else if(item_str == "steel") { target.item = Item::Steel; }
            else if(item_str == "armor") { target.item = Item::Armor; }
            else if(item_str == "tools") { target.item = Item::Tools; }
            else {
                engine::info("Invalid input, cancelled. Press enter to try again.");
                return;
            }
            engine::info(
                "Enter the amount of items to transfer "
                "(positive, may be a percentage, e.g. '3', '4.1%'):"
            );
            std::string amount_str = get_text_input();
            std::string amount_num_str = amount_str.ends_with("%")
                ? amount_str.substr(0, amount_str.size() - 1)
                : amount_str;
            f64 amount = NAN;
            sscanf(amount_num_str.c_str(), "%lf", &amount);
            bool valid = amount != NAN && amount != INFINITY && amount > 0;
            if(!valid) {
                engine::info("Invalid input, cancelled. Press enter to try again.");
                return;
            }
            target.action = action_str == "load"
                ? (amount_str.ends_with("%")
                    ? Carriage::LoadPercentage
                    : Carriage::LoadFixed
                )
                : (amount_str.ends_with("%")
                    ? Carriage::PutPercentage
                    : Carriage::PutFixed
                );
            if(amount_str.ends_with("%")) {
                target.amount.percentage = (f32) amount / 100.0;
            } else {
                target.amount.fixed = (u32) amount;
            }
            engine::info("Created a new stop at position [" 
                + std::to_string(carriage.targets.size()) + "]"
            );
            carriage.targets.push_back(target);
            return;
        }
        unsigned long long int target_i = ULLONG_MAX;
        sscanf(target_i_str.c_str(), "%llu", &target_i);
        if(target_i >= carriage.targets.size()) {
            engine::info("Invalid input, cancelled. Press enter to try again.");
            return;
        }
        engine::info("Enter what to do with the scheduled stop "
            "('swap', 'remove'):" 
        );
        std::string tgt_action = get_text_input();
        if(tgt_action == "remove") {
            carriage.targets.erase(carriage.targets.begin() + target_i);
            engine::info("Removed stop with position ["
                + std::to_string(target_i) + "]. (Please note that this may "
                "have changed the indices of other stops.)"
            );
            return;
        }
        if(tgt_action != "swap") {
            engine::info("Invalid input, cancelled. Press enter to try again.");
            return;
        }
        engine::info("Enter the position of a stop to swap the stop at position ["
            + std::to_string(target_i) + "] with:"
        );
        std::string swap_target_i_str = get_text_input();
        unsigned long long int swap_target_i = ULLONG_MAX;
        sscanf(swap_target_i_str.c_str(), "%llu", &swap_target_i);
        if(swap_target_i >= carriage.targets.size()) {
            engine::info("Invalid input, cancelled. Press enter to try again.");
            return;
        }
        std::swap(
            carriage.targets[target_i], carriage.targets[swap_target_i]
        );
        engine::info("Swapped the stops at positions ["
            + std::to_string(target_i) + "] and ["
            + std::to_string(swap_target_i) + "]."
        );
    }

    static const u64 carriage_buy_cost = 1000;
    static const u64 carr_spawn_d = 1;

    static void manage_carriages(
        i64 stable_x, i64 stable_z, const Terrain& terrain,
        CarriageManager& carriages, Balance& balance, 
        const ComplexBank& complexes, Toasts& toasts
    ) {
        std::string output;
        output += "===== Carriages =====";
        output += "\n    all carriages and their targets by their indices:";
        size_t carr_c = carriages.carriages.size();
        for(size_t carr_i = 0; carr_i < carr_c; carr_i += 1) {
            output += "\n[" + std::to_string(carr_i) + "]";
            const Carriage& carriage = carriages.carriages[carr_i];
            for(const Carriage::Target& target: carriage.targets) {
                output += " #" + std::to_string(target.complex.index);
            }
        }
        engine::info(output);
        engine::info(
            "Enter the index of a carriage to manage, "
            "or enter 'add' to create a new one:"
        );
        std::string carr_i_str = get_text_input();
        if(carr_i_str == "add") {
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
                engine::info(
                    "There is no valid location for a carriage nearby."
                );
                return;
            }
            if(!balance.pay_coins(carriage_buy_cost, toasts)) { return; }
            engine::info("Created a new carriage with index [" 
                + std::to_string(carriages.carriages.size()) + "]"
            );
            carriages.carriages.push_back((Carriage) { Carriage::Round, pos });
            return;
        }
        unsigned long long int carr_i = ULLONG_MAX;
        sscanf(carr_i_str.c_str(), "%llu", &carr_i);
        if(carr_i >= carriages.carriages.size()) {
            engine::info("Invalid input, cancelled. Press enter to try again.");
            return;
        }
        manage_carriage(carriages, carr_i, complexes);
    }

    void DefaultMode::update(
        const engine::Window& window, engine::Scene& scene, 
        const Renderer& renderer
    ) {
        (void) scene;
        (void) renderer;
        bool edit_carriages = this->selected.type == Selection::Building
            && window.was_pressed(engine::Key::Enter);
        if(edit_carriages) {
            i64 tile_x = (i64) this->selected.value.building.x; 
            i64 tile_z = (i64) this->selected.value.building.z; 
            const Building* building = this->terrain.building_at(tile_x, tile_z);
            if(building != nullptr && building->type == Building::Stable) {
                manage_carriages(
                    tile_x, tile_z, this->terrain, this->carriages, 
                    this->balance, this->complexes, this->toasts
                );
            }
        }
        if(!window.was_pressed(engine::Button::Left)) { return; }
        std::optional<size_t> carriage_i = this->carriages
            .find_selected_carriage(window.cursor_pos_ndc(), renderer);
        if(carriage_i.has_value()) {
            this->selected.type = Selection::Carriage;
            this->selected.value.carriage = *carriage_i;
            print_carriage_info(this->carriages, *carriage_i);
            return;
        }
        auto [s_tile_x, s_tile_z] = this->terrain.find_selected_terrain_tile(
            window.cursor_pos_ndc(), renderer, Vec<3>(0.5, 0, 0.5)
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
            this->selected.type = Selection::Building;
            this->selected.value.building.x = tile_x;
            this->selected.value.building.z = tile_z;
            return;
        }
        this->selected.type = Selection::None;
    }

    void DefaultMode::render(
        const engine::Window& window, engine::Scene& scene, 
        const Renderer& renderer
    ) {
        const engine::Texture& wireframe_texture = scene
            .get<engine::Texture>(ActionMode::wireframe_info_texture);
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
                for(const auto& [type, instances]: inst) {
                    const Building::TypeInfo& type_info = Building::types
                        .at((size_t) type);
                    type_info.render_buildings(
                        window, scene, renderer,
                        instances, true, &wireframe_texture
                    );
                }
            } break;
            case Selection::Building: {
                u64 tile_x = this->selected.value.building.x;
                u64 tile_z = this->selected.value.building.z;
                u64 chunk_x, chunk_z;
                const Building* building = this->terrain
                    .building_at((i64) tile_x, (i64) tile_z, &chunk_x, &chunk_z);
                const Building::TypeInfo& type_info = building->get_type_info();
                Mat<4> transform = this->terrain
                    .building_transform(*building, chunk_x, chunk_z);
                type_info.render_buildings(
                    window, scene, renderer,
                    std::array { transform }, true, &wireframe_texture
                );
            } break;
            case Selection::Carriage: {
                Carriage& carriage = this->carriages
                    .carriages[this->selected.value.carriage];
                carriage.render(
                    renderer, scene, window, true, &wireframe_texture
                );
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
        const Renderer& renderer
    ) {
        (void) scene;
        (void) renderer;
        auto [tile_x, tile_z] = this->terrain.find_selected_terrain_tile(
            window.cursor_pos_ndc(), renderer, Vec<3>(0, 0, 0)
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
            bool modified = this->modification_valid 
                && this->balance.pay_coins(cost, this->toasts);
            if(modified) {
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
        choose_building_type(window, this->selected_type, this->selected_conversion);
        const Building::TypeInfo& type_info = Building::types
            .at((size_t) this->selected_type);
        auto [tile_x, tile_z] = this->terrain.find_selected_terrain_tile(
            window.cursor_pos_ndc(), renderer,
            Vec<3>(type_info.width / 2.0, 0, type_info.height / 2.0)
        );
        this->selected_x = tile_x;
        this->selected_z = tile_z;
        this->placement_valid = this->terrain.valid_building_location(
            (i64) tile_x, (i64) tile_z, this->player.position, type_info
        );
        if(this->placement_valid && window.was_pressed(engine::Button::Left)) {
            i64 unemployment = this->terrain.compute_unemployment();
            bool allowed = unemployment >= (i64) type_info.workers
                && this->balance.pay_coins(type_info.cost, this->toasts);
            if(allowed) {
                place_building(
                    tile_x, tile_z, this->terrain, this->complexes,
                    this->selected_type, type_info, this->selected_conversion
                );
                this->carriages.refind_all_paths(this->complexes, this->terrain);
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
                this->carriages.refind_all_paths(this->complexes, this->terrain);
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
            this->carriages.refind_all_paths(this->complexes, this->terrain);
        } else if(has_path && window.was_pressed(engine::Button::Right)) {
            chunk.set_path_at(rel_x, rel_z, false);
            this->terrain.reload_chunk_at(chunk_x, chunk_z);
            this->carriages.refind_all_paths(this->complexes, this->terrain);
            this->balance.add_coins(path_removal_refund, this->toasts);
        }
    }

}