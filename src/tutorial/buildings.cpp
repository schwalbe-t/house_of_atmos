
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

    static std::shared_ptr<world::World> create_world(Settings&& settings) {
        auto world = std::make_shared<world::World>(
            std::move(settings), 64, 64
        );
        world->terrain.set_area_elevation(0, 0, 64, 64, 4);
        world->terrain.generate_foliage();
        world->populations.populations.push_back(
            world::Population(8, 26, world::PopulationName(world->terrain.rng))
        );
        world->terrain.remove_foliage_at(13, 32);
        world->terrain.remove_foliage_at(13, 33);
        world::ComplexId farmland_id = world->complexes.create_complex();
        world->terrain.place_building(
            world::Building::Farmland, 12, 30, farmland_id
        );
        world::Complex& farmland = world->complexes.get(farmland_id);
        farmland.add_member(12, 30, wheat_farm_cm);
        world->saving_allowed = false;
        world->player.character.type = &human::toddler;
        world->player.character.position = Vec<3>(13.75, 0, 32.5)
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
            scene->world->personal_horse.pos = Vec<3>(INFINITY, 0, INFINITY);
            for(auto& chunk: scene->world->terrain.all_loaded_chunks()) {
                chunk.interactables.clear();
            }
        };
        return {
            await_delay(update_scene, 1.0),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_2_father_0",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_2_father_1",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_2_father_2",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_2_father_3",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_2_prompt_4",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_2_prompt_5",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_2_father_6",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            await_dialogue_end(scene, update_scene),
            await_condition(
                scene, update_scene,
                [scene](auto& w) {
                    (void) w;
                    const world::Building* farmland = scene->world->terrain
                        .building_at(12, 30);
                    return farmland != nullptr
                        && farmland->complex.has_value()
                        && !scene->terrain_map.element()->hidden
                        && scene->terrain_map.selection_type() 
                            == world::TerrainMap::SelectionType::Complex
                        && scene->terrain_map.selection_value().complex.index
                            == farmland->complex->index;
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
                "dialogue_tutorial_2_father_7",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_2_father_8",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_2_prompt_9",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_2_prompt_10",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_prompt_name",
                "dialogue_tutorial_2_prompt_11",
                voice::popped, prompt_v_pitch, prompt_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_2_father_12",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            await_dialogue_end(scene, update_scene),
            await_condition(
                scene, update_scene,
                [scene](auto& w) {
                    (void) w;
                    const world::Building* farmland = scene->world->terrain
                        .building_at(12, 30);
                    if(farmland == nullptr || !farmland->complex.has_value()) {
                        return false;
                    }
                    const world::Complex& complex = scene->world
                        ->complexes.get(*farmland->complex);   
                    for(auto [item, freq]: complex.compute_throughput()) {
                        if(item != world::Item::Flour) { continue; }
                        return freq > 0.0;
                    }
                    return false;
                }
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_2_father_13",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_2_father_14",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_2_father_15",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            await_dialogue_end(scene, update_scene),
            await_condition(
                scene, update_scene,
                [scene](auto& w) {
                    (void) w;
                    const world::Building* farmland = scene->world->terrain
                        .building_at(12, 30);
                    if(farmland == nullptr || !farmland->complex.has_value()) {
                        return false;
                    }
                    const world::Complex& complex = scene->world
                        ->complexes.get(*farmland->complex);   
                    return complex.capacity(scene->world->terrain) > 100;
                }
            ),
            say_dialogue(
                scene, local,
                "dialogue_tutorial_father_name",
                "dialogue_tutorial_2_father_16",
                voice::voiced, father_v_pitch, father_v_speed
            ),
            await_dialogue_end(scene, update_scene),
            await_condition(
                scene, update_scene,
                [scene](auto& w) {
                    (void) w;
                    const world::Building* farmland = scene->world->terrain
                        .building_at(12, 30);
                    return farmland != nullptr
                        && farmland->complex.has_value()
                        && !scene->terrain_map.element()->hidden
                        && scene->terrain_map.selection_type() 
                            == world::TerrainMap::SelectionType::Complex
                        && scene->terrain_map.selection_value().complex.index
                            == farmland->complex->index;
                }
            ),
            await_condition(
                scene, update_scene,
                [scene](auto& w) {
                    (void) w;
                    return scene->terrain_map.element()->hidden;
                },
                [scene, after](engine::Window& window) {
                    after->settings = scene->world->settings;
                    window.set_scene(create_logistics_scene(after));
                }
            )
        };
    }

    

    std::shared_ptr<engine::Scene> create_buildings_scene(
        std::shared_ptr<world::World> world_after
    ) {
        auto world = create_world(Settings(world_after->settings));
        auto scene = std::make_shared<world::Scene>(world);
        scene->cutscene.append(create_cutscene(scene, world_after));
        return scene;
    }

}