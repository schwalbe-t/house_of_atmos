
#include "tutorial.hpp"
#include "common.hpp"

namespace houseofatmos::tutorial {

    static const world::Complex::Member bread_plaza_cm = world::Complex::Member({
        world::Conversion(
            { { 1, world::Item::Bread } }, 
            { { 3, world::Item::Coins } }, 
            0.1
        )
    });

    static const world::Complex::Member bread_factory_cm = world::Complex::Member({
        world::Conversion(
            { { 1, world::Item::Flour } }, 
            { { 2, world::Item::Bread } }, 
            2.0 
        )
    });



    static const world::Complex::Member wheat_farm_cm = world::Complex::Member({
        world::Conversion(
            {}, 
            { { 10, world::Item::Wheat } }, 
            10.0 
        )
    });

    static world::World create_world(Settings&& settings) {
        auto world = world::World(std::move(settings), 128, 128);
        // west river
        world.terrain.set_area_elevation(0, 0, 128, 128, 4);
        world.terrain.set_area_elevation(37, 0, 45, 128, 3);
        world.terrain.set_area_elevation(38, 0, 44, 128, 1);
        world.terrain.set_area_elevation(39, 0, 43, 128, 0);
        world.terrain.set_area_elevation(40, 0, 42, 128, -1);
        world.terrain.set_area_elevation(41, 0, 41, 128, -2);
        // east river
        world.terrain.set_area_elevation(89, 0, 95, 128,  1);
        world.terrain.set_area_elevation(90, 0, 94, 128,  0);
        world.terrain.set_area_elevation(91, 0, 93, 128, -1);
        world.terrain.set_area_elevation(92, 0, 92, 128, -2);
        // foliage
        world.terrain.generate_foliage();
        // structures east of rivers
        world.terrain.place_building(world::Building::Mansion, 12, 49);
        world.terrain.remove_foliage_at(13, 53);
        for_line(13, 50, 13, 52, set_path_at(world.terrain));
        for_line(7, 52, 21, 52, set_path_at(world.terrain));
        for_line(21, 52, 21, 62, set_path_at(world.terrain));
        for_line(21, 62, 35, 62, set_path_at(world.terrain));
        for_line(35, 52, 35, 62, set_path_at(world.terrain));
        for_line(36, 52, 45, 52, remove_foliage_from(world.terrain));
        world.terrain.bridges.push_back((world::Bridge) {
            world::Bridge::Stone, 36, 52, 45, 52, 4
        });
        // structures between the two rivers
        for_line(46, 52, 63, 52, set_path_at(world.terrain));
        for_line(63, 52, 63, 76, set_path_at(world.terrain));
        for_line(58, 79, 63, 79, set_path_at(world.terrain));
        for_line(58, 79, 58, 84, set_path_at(world.terrain));
        for_line(62, 80, 62, 86, set_path_at(world.terrain));
        for_line(62, 86, 64, 86, set_path_at(world.terrain));
        for_line(65, 77, 71, 77, set_path_at(world.terrain));
        for_line(68, 78, 68, 80, set_path_at(world.terrain));
        for_line(68, 80, 87, 80, set_path_at(world.terrain));
        for_line(69, 80, 69, 84, set_path_at(world.terrain));
        world.terrain.place_building(world::Building::House, 64, 73);
        world.terrain.place_building(world::Building::House, 64, 75);
        world.terrain.place_building(world::Building::House, 66, 76);
        world.terrain.place_building(world::Building::House, 68, 76);
        world.terrain.place_building(world::Building::House, 70, 76);
        world.terrain.place_building(world::Building::House, 59, 78);
        world.terrain.place_building(world::Building::House, 61, 78);
        world.terrain.place_building(world::Building::House, 66, 78);
        world.terrain.place_building(world::Building::House, 57, 79);
        world.terrain.place_building(world::Building::House, 69, 79);
        world.terrain.place_building(world::Building::House, 71, 79);
        world.terrain.place_building(world::Building::House, 73, 79);
        world.terrain.place_building(world::Building::House, 59, 80);
        world.terrain.place_building(world::Building::House, 61, 80);
        world.terrain.place_building(world::Building::House, 63, 80);
        world.terrain.place_building(world::Building::House, 67, 80);
        world.terrain.place_building(world::Building::House, 57, 81);
        world.terrain.place_building(world::Building::House, 71, 81);
        world.terrain.place_building(world::Building::House, 59, 82);
        world.terrain.place_building(world::Building::House, 61, 82);
        world.terrain.place_building(world::Building::House, 68, 82);
        world.terrain.place_building(world::Building::House, 70, 82);
        world.terrain.place_building(world::Building::House, 57, 83);
        world.terrain.place_building(world::Building::House, 61, 84);
        world.terrain.place_building(world::Building::House, 68, 84);
        world.terrain.place_building(world::Building::House, 70, 84);
        world.terrain.place_building(world::Building::House, 61, 86);
        world.terrain.place_building(world::Building::House, 62, 87);
        world.terrain.place_building(world::Building::House, 64, 87); 
        world::ComplexId village_center_id = world.complexes.create_complex();
        world.terrain.place_building(
            world::Building::Plaza, 62, 77, village_center_id
        );
        world.terrain.place_building(
            world::Building::Factory, 63, 82, village_center_id
        );
        world.terrain.place_building(
            world::Building::Factory, 63, 83, village_center_id
        );
        world.terrain.place_building(
            world::Building::Factory, 63, 84, village_center_id
        );
        world.terrain.place_building(
            world::Building::Factory, 63, 85, village_center_id
        );
        world::Complex& village_center = world.complexes.get(village_center_id);
        village_center.add_member(62, 77, bread_plaza_cm);
        village_center.add_member(63, 82, bread_factory_cm);
        village_center.add_member(63, 83, bread_factory_cm);
        village_center.add_member(63, 84, bread_factory_cm);
        village_center.add_member(63, 85, bread_factory_cm);
        // structres west of the rivers
        for_line(96, 80, 111, 80, set_path_at(world.terrain));
        world.terrain.set_path_at(111, 79);
        world.terrain.set_path_at(111, 77);
        world.terrain.set_path_at(111, 75);
        for_line(111, 66, 111, 73, set_path_at(world.terrain));
        world::ComplexId farmland_center_id = world.complexes.create_complex();
        world.terrain.place_building(
            world::Building::Farmland, 109, 67, farmland_center_id
        );
        world.terrain.place_building(
            world::Building::Farmland, 109, 70, farmland_center_id
        );
        world.terrain.place_building(
            world::Building::Farmland, 112, 67, farmland_center_id
        );
        world.terrain.place_building(
            world::Building::Farmland, 112, 70, farmland_center_id
        );
        world::Complex& farmland_center = world.complexes.get(farmland_center_id);
        farmland_center.add_member(109, 67, wheat_farm_cm);
        farmland_center.add_member(109, 70, wheat_farm_cm);
        farmland_center.add_member(112, 67, wheat_farm_cm);
        farmland_center.add_member(112, 70, wheat_farm_cm);
        // other stuff
        world.saving_allowed = false;
        world.player.character.position = Vec<3>(13.5, 0, 50.5)
            * world.terrain.units_per_tile();
        world.balance.coins = 0;
        world.personal_horse.pos = Vec<3>(14.25, 0, 52.25)
            * world.terrain.units_per_tile();
        return world;
    }



    static Cutscene::Section await_condition(
        std::shared_ptr<world::Scene> scene,
        std::shared_ptr<world::World> after,
        std::function<bool (engine::Window&)> cond,
        std::function<void (engine::Window&)> handler 
            = [](auto& w) { (void) w; }
    ) {
        return Cutscene::Section(
            INFINITY,
            [scene, after, cond](engine::Window& window) {
                if(window.was_pressed(engine::Key::Tab)) {
                    window.set_scene(std::make_shared<world::Scene>(after));
                }
                if(cond(window)) {
                    scene->cutscene.advance();
                }
            },
            std::move(handler)
        );
    }

    static Cutscene::Section await_distance_to_point(
        std::shared_ptr<world::Scene> scene,
        std::shared_ptr<world::World> after,
        Vec<3> point, f64 max_distance,
        std::function<void (engine::Window&)> handler 
            = [](auto& w) { (void) w; }
    ) {
        return Cutscene::Section(
            INFINITY,
            [scene, after, point, max_distance](engine::Window& window) {
                if(window.was_pressed(engine::Key::Tab)) {
                    window.set_scene(std::make_shared<world::Scene>(after));
                }
                Vec<3> offset = point
                    - scene->world->player.character.position;
                if(offset.len() <= max_distance) {
                    scene->cutscene.advance();
                }
            },
            std::move(handler)
        );
    }

    static Cutscene::Section say_dialogue(
        std::shared_ptr<world::Scene> scene, const engine::Localization* local,
        std::string name_key, std::string text_key,
        const Dialogue::Voice& voice, f64 v_pitch, f64 v_speed
    ) {
        return Cutscene::Section(
            0.0,
            [](auto& w) { (void) w; },
            [
                scene, local, name_key, text_key,
                voice = &voice, v_pitch, v_speed
            ](auto& w) {
                (void) w;
                scene->dialogues.say(Dialogue(
                    std::string(local->text(name_key)),
                    std::string(local->text(text_key)),
                    voice, v_pitch, v_speed,
                    { 0, 0, 0 }, INFINITY
                ));
            }
        );
    }

    static Cutscene::Section await_dialogue_end(
        std::shared_ptr<world::Scene> scene,
        std::shared_ptr<world::World> after,
        std::function<void (engine::Window&)> handler 
            = [](auto& w) { (void) w; }
    ) {
        return Cutscene::Section(
            INFINITY,
            [scene, after](engine::Window& window) {
                if(window.was_pressed(engine::Key::Tab)) {
                    window.set_scene(std::make_shared<world::Scene>(after));
                }
                if(scene->dialogues.is_empty()) {
                    scene->cutscene.advance();
                }
            },
            std::move(handler)
        );
    }

    static Cutscene create_cutscene(
        std::shared_ptr<world::Scene> scene,
        std::shared_ptr<world::World> after
    ) {
        // this is only legal because the previous scene already loads this :/
        const engine::Localization* local = &scene
            ->get<engine::Localization>(scene->world->settings.localization());
        scene->characters.push_back(Character(
            &human::male, &human::father, { 0, 0, 0 },
            (u64) human::Animation::Stand
        ));
        Character* father = &scene->characters.back();
        father->position = Vec<3>(13.25, 0, 52.5) 
            * scene->world->terrain.units_per_tile()
            + Vec<3>(0.0, 4.0, 0.0);
        father->face_in_direction({ 1, 0, 0 });
        return {
            await_distance_to_point(
                scene, after,
                father->position, 3.0
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_1_father_0",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            await_dialogue_end(scene, after),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_1_father_1",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            await_dialogue_end(scene, after),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_1_father_2",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            await_dialogue_end(
                scene, after,
                [scene](auto& w) {
                    (void) w;
                    scene->world->balance.add_coins(99000, scene->toasts);
                }
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_1_prompt_3",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_1_prompt_4",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            await_distance_to_point(
                scene, after,
                Vec<3>(63.5, 0, 78.5) 
                    * scene->world->terrain.units_per_tile()
                    + Vec<3>(0.0, 4.0, 0.0), 
                20.0
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_instructions_name",
                "dialogue_tutorial_1_instructions_5",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_instructions_name",
                "dialogue_tutorial_1_instructions_6",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            await_distance_to_point(
                scene, after,
                Vec<3>(87.5, 0, 80.5) 
                    * scene->world->terrain.units_per_tile()
                    + Vec<3>(0.0, 4.0, 0.0), 
                10.0
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_instructions_name",
                "dialogue_tutorial_1_instructions_7",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_1_prompt_8",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_1_prompt_9",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            await_condition(
                scene, after,
                [scene](auto& w) {
                    (void) w;
                    return scene->world->terrain.bridges.size() >= 2;
                }
            ),
            await_distance_to_point(
                scene, after,
                Vec<3>(111.5, 0, 72.5) 
                    * scene->world->terrain.units_per_tile()
                    + Vec<3>(0.0, 4.0, 0.0), 
                10.0
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_instructions_name",
                "dialogue_tutorial_1_instructions_10",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_instructions_name",
                "dialogue_tutorial_1_instructions_11",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_1_prompt_12",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_1_prompt_13",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_1_prompt_14",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_1_prompt_15",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            await_condition(
                scene, after,
                [scene](auto& w) {
                    (void) w;
                    std::optional<world::ComplexId> farmland_id
                        = scene->world->complexes.closest_to(111, 69);
                    if(!farmland_id.has_value()) { return false; }
                    world::Complex& farmland = scene->world->complexes
                        .get(*farmland_id);
                    for(auto [item, freq]: farmland.compute_throughput()) {
                        if(item != world::Item::Flour) { continue; }
                        return freq > 0.0;
                    }
                    return false;
                }
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_instructions_name",
                "dialogue_tutorial_1_instructions_16",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_1_prompt_17",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_1_prompt_18",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            await_condition(
                scene, after,
                [scene](auto& w) {
                    (void) w;
                    for(u64 z = 72; z <= 80; z += 1) {
                        if(!scene->world->terrain.path_at(111, z)) {
                            return false;
                        }
                    }
                    return true;
                }
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_instructions_name",
                "dialogue_tutorial_1_instructions_19",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_instructions_name",
                "dialogue_tutorial_1_instructions_20",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            await_condition(
                scene, after,
                [scene](auto& w) {
                    (void) w;
                    u64 chunk_w = scene->world->terrain.width_in_chunks();
                    u64 chunk_h = scene->world->terrain.height_in_chunks();
                    for(u64 ch_x = 0; ch_x < chunk_w; ch_x += 1) {
                        for(u64 ch_z = 0; ch_z < chunk_h; ch_z += 1) {
                            const world::Terrain::ChunkData& chunk
                                = scene->world->terrain.chunk_at(ch_x, ch_z);
                            for(const auto& building: chunk.buildings) {
                                bool is_stable = building.type 
                                    == world::Building::Stable;
                                if(is_stable) { return true; }
                            }
                        }
                    }
                    return false;
                }
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_instructions_name",
                "dialogue_tutorial_1_instructions_21",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_1_prompt_22",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_1_prompt_23",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            await_condition(
                scene, after,
                [scene](auto& w) {
                    (void) w;
                    return scene->world->carriages.carriages.size() > 0;
                }
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_instructions_name",
                "dialogue_tutorial_1_instructions_24",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_1_prompt_25",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_1_prompt_26",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_1_prompt_27",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_1_prompt_28",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_1_prompt_29",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, after),
            await_condition(
                scene, after,
                [scene](auto& w) {
                    (void) w;
                    std::optional<world::ComplexId> village_id
                        = scene->world->complexes.closest_to(63, 80);
                    if(!village_id.has_value()) { return false; }
                    world::Complex& village = scene->world->complexes
                        .get(*village_id);
                    return village.stored_count(world::Item::Flour) > 0;
                }
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_1_prompt_30",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(
                scene, after,
                [after, local](engine::Window& window) {
                    auto new_scene = std::make_shared<world::Scene>(after);
                    new_scene->dialogues.say(Dialogue(
                        std::string(local->text("dialogue_tutorial_prompt_name")),
                        std::string(local->text("dialogue_tutorial_2_prompt_0")),
                        &voice::popped, prompt_v_pitch, prompt_v_speed,
                        { 0, 0, 0 }, INFINITY
                    ));
                    window.set_scene(new_scene);
                }
            )
        };
    }

    

    std::shared_ptr<engine::Scene> create_grownup_scene(
        std::shared_ptr<world::World> world_after
    ) {
        auto world = std::make_shared<world::World>(
            create_world(Settings(world_after->settings))
        );
        auto scene = std::make_shared<world::Scene>(std::move(world));
        scene->cutscene.append(create_cutscene(scene, std::move(world_after)));
        return scene;
    }

}