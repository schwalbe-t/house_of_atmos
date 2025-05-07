
#include "tutorial.hpp"
#include "common.hpp"

namespace houseofatmos::tutorial {

    static world::Complex::Member wheat_farm_cm = world::Complex::Member({
        world::Conversion(
            {}, 
            { { 8, world::Item::Wheat } }, 
            8.0 
        )
    });

    static world::Complex::Member flour_windmill_cm = world::Complex::Member({
        world::Conversion(
            { { 4, world::Item::Wheat } }, 
            { { 1, world::Item::Flour } }, 
            1.0
        )
    });

    static world::Complex::Member bread_bakery_cm = world::Complex::Member({
        world::Conversion( 
            { { 4, world::Item::Flour } }, 
            { { 4, world::Item::Bread } }, 
            4.0
        )
    });

    static std::shared_ptr<world::World> create_world(Settings&& settings) {
        auto world = std::make_shared<world::World>(
            std::move(settings), 64, 64
        );
        world->terrain.set_area_elevation(0, 0, 64, 64, 4);
        world->terrain.generate_foliage();
        world->populations.populations.push_back(
            world::Population(8, 26, world::PopulationName(world->terrain.rng))
        );
        world->populations.populations.push_back(
            world::Population(40, 32, world::PopulationName(world->terrain.rng))
        );
        world->terrain.remove_foliage_at(13, 32);
        world->terrain.remove_foliage_at(13, 33);
        world::ComplexId farmland_id = world->complexes.create_complex();
        world->terrain.place_building(
            world::Building::Farmland, 12, 30, farmland_id
        );
        world->terrain.place_building(
            world::Building::Windmill, 14, 29, farmland_id
        );
        world->terrain.place_building(
            world::Building::Storage, 16, 29, farmland_id
        );
        for_line(14, 31, 19, 31, set_path_at(world->terrain));
        world::Complex& farmland = world->complexes.get(farmland_id);
        farmland.add_member(12, 30, wheat_farm_cm);
        farmland.add_member(14, 29, flour_windmill_cm);
        world::ComplexId bakery_id = world->complexes.create_complex();
        world->terrain.place_building(
            world::Building::CommissaryWorks, 35, 32, bakery_id
        );
        world->terrain.place_building(
            world::Building::Storage, 37, 31, bakery_id
        );
        for_line(33, 33, 37, 33, set_path_at(world->terrain));
        world::Complex& bakery = world->complexes.get(bakery_id);
        bakery.add_member(35, 32, bread_bakery_cm);
        bakery.add_member(37, 31, world::Complex::Member({}));
        world->saving_allowed = false;
        world->player.character.position = Vec<3>(13.75, 0, 32.5)
            * world->terrain.units_per_tile();
        world->personal_horse.pos = Vec<3>(14.0, 0.0, 33.0)
            * world->terrain.units_per_tile();
        world->balance.set_coins_silent(Balance::infinite_coins);
        return world;
    }



    static Cutscene create_cutscene(
        std::shared_ptr<world::Scene> scene,
        std::shared_ptr<world::World> after
    ) {
        // this is only legal because the previous scene already loads this :/
        const engine::Localization* local 
            = &scene->get(scene->world->settings.localization());
        scene->characters.push_back(Character(
            &human::male, &human::father, { 0, 0, 0 },
            (u64) human::Animation::Stand
        ));
        Character* father = &scene->characters.back();
        father->position = Vec<3>(13.25, 0, 32.5) 
            * scene->world->terrain.units_per_tile();
        father->position.y() = scene->world->terrain
            .elevation_at(father->position);
        father->face_in_direction({ 1, 0, 0 });
        auto update_scene = [scene](engine::Window& window) {
            (void) window;
            for(auto& chunk: scene->world->terrain.all_loaded_chunks()) {
                chunk.interactables.clear();
            }
        };
        return {
            await_delay(update_scene, 1.0),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_3_father_0",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_3_father_1",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_3_prompt_2",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_3_prompt_3",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_3_father_4",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            await_dialogue_end(scene, update_scene),
            await_condition(
                scene, update_scene,
                [scene](auto& w) {
                    (void) w;
                    return !scene->terrain_map.element()->hidden;
                }
            ),
            await_condition(
                scene, update_scene,
                [scene](auto& w) {
                    (void) w;
                    return scene->terrain_map.element()->hidden;
                }
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_3_father_5",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_3_prompt_6",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_3_prompt_7",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(scene, update_scene),
            await_condition(
                scene, update_scene,
                [scene](auto& w) {
                    (void) w;
                    const world::Building* bakery = scene->world->terrain
                        .building_at(35, 32);
                    if(bakery == nullptr || !bakery->complex.has_value()) {
                        return false;
                    }
                    return world::AgentPath<world::CarriageNetwork>::find(
                        scene->world->carriages.network,
                        { 16, 31 }, *bakery->complex
                    ).has_value();
                }
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_3_father_8",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_3_father_9",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_3_father_10",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            await_dialogue_end(scene, update_scene),
            await_condition(
                scene, update_scene,
                [scene](auto& w) {
                    (void) w;
                    u64 ch_width = scene->world->terrain.width_in_chunks();
                    u64 ch_height = scene->world->terrain.height_in_chunks();
                    for(u64 ch_x = 0; ch_x < ch_width; ch_x += 1) {
                        for(u64 ch_z = 0; ch_z < ch_height; ch_z += 1) {
                            const world::Terrain::ChunkData& chunk
                                = scene->world->terrain.chunk_at(ch_x, ch_z);
                            for(const auto& building: chunk.buildings) {
                                if(building.type == world::Building::Stable) {
                                    return true;
                                }
                            }
                        }
                    }
                    return false;
                }
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_3_prompt_11",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_3_father_12",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            await_dialogue_end(scene, update_scene),
            await_condition(
                scene, update_scene,
                [scene](auto& w) {
                    (void) w;
                    for(const auto& carr: scene->world->carriages.agents) {
                        if(carr.type != world::Carriage::Round) { continue; }
                        return true;
                    }
                    return false;
                }
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_3_prompt_13",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_3_prompt_14",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_3_father_15",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            await_dialogue_end(scene, update_scene),
            await_condition(
                scene, update_scene,
                [scene](auto& w) {
                    (void) w;
                    const world::Building* bakery = scene->world->terrain
                        .building_at(35, 32);
                    if(bakery == nullptr || !bakery->complex.has_value()) {
                        return false;
                    }
                    const world::Complex& bakery_c = scene->world->complexes
                        .get(*bakery->complex);
                    for(const auto& [item, count]: bakery_c.stored_items()) {
                        if(item != world::Item::Flour) { continue; }
                        return count > 0;
                    }
                    return false;
                }
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_3_prompt_16",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            await_dialogue_end(
                scene, update_scene,
                [scene, after](engine::Window& window) {
                    after->settings = scene->world->settings;
                    window.set_scene(create_discovery_scene(after));
                }
            )
        };
    }

    

    std::shared_ptr<engine::Scene> create_logistics_scene(
        std::shared_ptr<world::World> world_after
    ) {
        auto world = create_world(Settings(world_after->settings));
        auto scene = std::make_shared<world::Scene>(world);
        scene->cutscene.append(create_cutscene(scene, world_after));
        return scene;
    }

}