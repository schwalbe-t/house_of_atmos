
#include "actionmode.hpp"
#include <iostream>

namespace houseofatmos::outside {

    void ActionMode::choose_current(
        const engine::Window& window, Terrain& terrain, const Player& player,
        std::unique_ptr<ActionMode>& current
    ) {
        for(size_t type_i = 0; type_i < ActionMode::keys.size(); type_i += 1) {
            engine::Key key = ActionMode::keys.at(type_i);
            ActionMode::Type type = (ActionMode::Type) type_i;
            if(!window.was_pressed(key)) { continue; }
            if(current->get_type() == type) {
                current = std::make_unique<DefaultMode>();
                break;
            }
            switch(type) {
                case Default: current = std::make_unique<DefaultMode>(); break;
                case Terraform: current = std::make_unique<TerraformMode>(terrain); break;
                case Construction: current = std::make_unique<ConstructionMode>(terrain, player); break;
                case Demolition: current = std::make_unique<DemolitionMode>(terrain); break;
                case Pathing: current = std::make_unique<PathingMode>(terrain); break;
                default: engine::warning(
                    "Unhandled 'ActionMode::Type' in 'ActionMode::choose_current'"
                );
            }
            break;
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
                "(one of: 'house', 'farmland', 'mineshaft', 'windmill', 'factory')"
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
                        { { 40, Item::Wheat } }, 
                        10.0
                    ) };
                    engine::info("Selected farmland (-> 40 Wheat / 10 s)");
                    return;
                }
                if(produce_name == "barley") {
                    type = Building::Farmland;
                    conversions = { Conversion(
                        {}, 
                        { { 40, Item::Barley } }, 
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
            engine::info("Invalid input, cancelled. Press enter to try again.");
        }
    }

    static void place_building(
        u64 tile_x, u64 tile_z, Terrain& terrain,
        Building::Type type, const Building::TypeInfo& type_info,
        const std::vector<Conversion>& conversions
    ) {
        u64 chunk_x = tile_x / terrain.tiles_per_chunk();
        u64 chunk_z = tile_z / terrain.tiles_per_chunk();
        Terrain::ChunkData& chunk = terrain.chunk_at(chunk_x, chunk_z);
        std::optional<ComplexId> complex_id = std::nullopt;
        // if(conversions.size() > 0) {
        //     complex_id = terrain.complexes.closest_to(tile_x, tile_z);
        //     if(!complex_id.has_value()) {
        //         complex_id = terrain.complexes.create_complex();
        //     }
        //     Complex& complex = terrain.complexes.get(*complex_id);
        //     if(complex.distance_to(tile_x, tile_z) > Complex::max_building_dist) {
        //         complex_id = terrain.complexes.create_complex();
        //     }
        //     complex.add_member(tile_x, tile_z, Complex::Member(conversions));
        // }
        engine::debug("Building was added to complex " + (complex_id.has_value()? std::to_string(complex_id->index) : "<none>"));
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
        bool was_placed = this->placement_valid
            && window.was_pressed(engine::Button::Left)
            && balance.pay_coins(type_info.cost);
        if(was_placed) {
            place_building(
                tile_x, tile_z, this->terrain, this->selected_type, 
                type_info, this->selected_conversion
            );
        }
    }

    void ConstructionMode::render(
        const engine::Window& window, engine::Scene& scene, 
        const Renderer& renderer
    ) {
        const engine::Texture& wireframe_texture = this->placement_valid
            ? scene.get<engine::Texture>(ActionMode::wireframe_valid_texture)
            : scene.get<engine::Texture>(ActionMode::wireframe_error_texture);
        const Building::TypeInfo& type_info = Building::types
            .at((size_t) this->selected_type);
        engine::Model& model = scene.get<engine::Model>(type_info.model);
        Vec<3> offset = Vec<3>(
            this->selected_x + type_info.offset_x, 0, this->selected_z + type_info.offset_z
        ) * this->terrain.units_per_tile();
        offset.y() = this->terrain.elevation_at(this->selected_x, this->selected_z);
        Mat<4> transform = Mat<4>::translate(offset);
        if(type_info.animation.has_value()) {
            const engine::Animation& animation = model.animation(*type_info.animation);
            renderer.render(
                model, transform, 
                animation,
                fmod(window.time() * type_info.animation_speed, animation.length()),
                true, &wireframe_texture
            );
        } else {
            renderer.render(
                model, std::array { transform }, true, &wireframe_texture
            );
        }
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
            Terrain::ChunkData& chunk = this->terrain
                .chunk_at(this->selected_chunk_x, this->selected_chunk_z);
            // if(this->selected->complex.has_value()) {
            //     Complex& complex = this->terrain.complexes
            //         .get(*selected->complex);
            //     complex.remove_member(tile_x, tile_z);
            //     if(complex.member_count() == 0) {
            //         this->terrain.complexes.delete_complex(*selected->complex);
            //     }
            // }
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
        }
    }

    void DemolitionMode::render(
        const engine::Window& window, engine::Scene& scene, 
        const Renderer& renderer
    ) {
        if(this->selected != nullptr) {
            const engine::Texture& wireframe_texture = scene
                .get<engine::Texture>(ActionMode::wireframe_error_texture);
            const Building::TypeInfo& type_info = Building::types
                .at((size_t) this->selected->type);
            engine::Model& model = scene.get<engine::Model>(type_info.model);
            Vec<3> chunk_offset = Vec<3>(
                this->selected_chunk_x, 0, this->selected_chunk_z
            ) * this->terrain.tiles_per_chunk() * this->terrain.units_per_tile();
            Vec<3> offset = chunk_offset + Vec<3>(
                this->selected->x + type_info.offset_x, 
                0, 
                this->selected->z + type_info.offset_z
            ) * this->terrain.units_per_tile();
            offset.y() = this->terrain.elevation_at(
                this->selected_chunk_x * this->terrain.tiles_per_chunk()
                    + this->selected->x, 
                this->selected_chunk_z * this->terrain.tiles_per_chunk()
                    + this->selected->z
            );
            Mat<4> transform = Mat<4>::translate(offset);
            if(type_info.animation.has_value()) {
                const engine::Animation& animation = model
                    .animation(*type_info.animation);
                renderer.render(
                    model, transform, 
                    animation,
                    fmod(window.time() * type_info.animation_speed, animation.length()),
                    true, &wireframe_texture
                );
            } else {
                renderer.render(
                    model, std::array { transform }, true, &wireframe_texture
                );
            }
        }
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