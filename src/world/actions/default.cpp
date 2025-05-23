
#include "common.hpp"
#include "../../ui_util.hpp"

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
                bool is_valid = x >= 0 && z >= 0
                    && (u64) x < world.terrain.width_in_tiles()
                    && (u64) z < world.terrain.height_in_tiles()
                    && world.carriages.network.is_passable({ (u64) x, (u64) z });
                if(!is_valid) { continue; }
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
            Carriage(carriage_type, pos, rng, world.settings)
        );
        speaker.position = pos;
        speaker.pitch = 1.0;
        speaker.play(scene.get(sound::horse));
    }

    static void summon_locomotive(
        engine::Scene& scene, World& world, engine::Speaker& speaker, 
        u64 depot_cx, u64 depot_cz, Toasts& toasts,
        Train::LocomotiveType loco_type
    ) {
        TrackPosition pos;
        bool found_pos = false;
        const Building::TypeInfo& depot
            = Building::types().at((size_t) Building::TrainDepot);
        i64 start_x = depot_cx - depot.width / 2;
        i64 end_x = start_x + depot.width;
        i64 z = (depot_cz - depot.width / 2) + depot.height;
        for(i64 x = start_x; x < end_x; x += 1) {
            if(x < 0 || (u64) x >= world.terrain.width_in_tiles()) { continue; }
            if(z < 0 || (u64) z >= world.terrain.height_in_tiles()) { continue; }
            u64 chunk_x = (u64) x / world.terrain.tiles_per_chunk();
            u64 chunk_z = (u64) z / world.terrain.tiles_per_chunk();
            u64 chunk_rx = (u64) x % world.terrain.tiles_per_chunk();
            u64 chunk_rz = (u64) z % world.terrain.tiles_per_chunk();
            const Terrain::ChunkData& chunk = world.terrain
                .chunk_at(chunk_x, chunk_z);
            for(size_t pc_i = 0; pc_i < chunk.track_pieces.size(); pc_i += 1) {
                const TrackPiece& piece = chunk.track_pieces[pc_i];
                if(piece.x != chunk_rx || piece.z != chunk_rz) { continue; }
                auto piece_id = TrackPieceId(chunk_x, chunk_z, pc_i);
                auto piece_node = world.trains.network.graph.at(piece_id);
                bool is_ascending = piece_node.connected_low.size() == 0;
                pos = TrackPosition(
                    piece_id, 
                    is_ascending
                        ? TrackPiece::Direction::Ascending 
                        : TrackPiece::Direction::Descending, 
                    0.0
                );
                found_pos = true;
                break;
            }
            if(found_pos) { break; }
        }
        if(!found_pos) {
            toasts.add_error("toast_no_valid_train_location", {});
            return;
        }
        const Train::LocomotiveTypeInfo& loco_info = Train::locomotive_types()
            .at((size_t) loco_type);
        u64 cost = loco_info.cost;
        if(!world.balance.pay_coins(cost, toasts)) { return; }
        world.trains.agents.push_back(Train(
            loco_type, 
            std::vector<Train::CarPosition>(
                loco_info.loco_cars.size(), Train::CarPosition(pos, pos)
            ), 
            world.settings
        ));
        speaker.position = pos.in_world(world.trains.network);
        speaker.pitch = loco_info.whistle_pitch;
        speaker.play(scene.get(sound::train_whistle));
    }

    static const i64 boat_spawn_d = 5;

    static void summon_boat(
        World& world, u64 yard_cx, u64 yard_cz, Toasts& toasts,
        Boat::Type boat_type
    ) {
        Vec<3> pos;
        bool found_pos = false;
        i64 start_x = yard_cx - boat_spawn_d;
        i64 start_z = yard_cz - boat_spawn_d;
        i64 end_x = yard_cx + boat_spawn_d;
        i64 end_z = yard_cz + boat_spawn_d;
        for(i64 x = start_x; x <= end_x; x += 1) {
            for(i64 z = start_z; z <= end_z; z += 1) {
                bool is_valid = x >= 1 && z >= 1
                    && (u64) x < world.terrain.width_in_tiles()
                    && (u64) z < world.terrain.height_in_tiles()
                    && world.boats.network.is_passable({ (u64) x, (u64) z });
                if(!is_valid) { continue; }
                pos = Vec<3>(x, 0, z) * world.terrain.units_per_tile();
                pos.y() = world.terrain.elevation_at(pos);
                found_pos = true;
                break;
            }
        }
        if(!found_pos) {
            toasts.add_error("toast_no_valid_boat_location", {});
            return;
        }
        u64 cost = Boat::types().at((size_t) boat_type).cost;
        if(!world.balance.pay_coins(cost, toasts)) { return; }
        world.boats.agents.push_back(Boat(boat_type, pos));
    }

    template<typename C>
    static ui::Element create_agent_selector(
        const engine::Localization& local,
        std::string_view local_title, 
        std::span<const C> choices,
        std::function<bool (const C&)> choice_unlocked,
        std::string_view (*local_choice_name)(const C&),
        const ui::Background* (*choice_icon)(const C&),
        std::function<void (const C&, size_t)> handler
    ) {
        const std::string& title = local.text(local_title);
        ui::Element selector = ui_util::create_selection_container(title)
            .with_pos(
                ui::horiz::in_window_fract(0.95), ui::vert::in_window_fract(0.5)
            )
            .as_movable();
        for(size_t ci = 0; ci < choices.size(); ci += 1) {
            const C& choice = choices[ci];
            if(!choice_unlocked(choice)) { continue; }
            selector.children.push_back(ui_util::create_selection_item(
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
                        [](auto ct) { (void) ct; return true; },
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
                        [this](const Train::LocomotiveTypeInfo& ct) { 
                            return !ct.req_reward.has_value() 
                                || this->world->research
                                    .is_unlocked(*ct.req_reward);
                        },
                        [](auto ct) { return ct.local_name; },
                        [](auto ct) { return ct.icon; },
                        [scene = &scene, this, asx, asz](auto ct, auto ti) {
                            (void) ct;
                            summon_locomotive(
                                *scene, *this->world, this->speaker,
                                asx, asz, this->toasts,
                                (Train::LocomotiveType) ti
                            );
                        }
                    );
                    break;
                }
                case Building::ShipYard: {
                    this->selector.origin_x = asx;
                    this->selector.origin_z = asz;
                    *this->selector.element 
                        = create_agent_selector<Boat::TypeInfo>(
                        this->local, "ui_boat", Boat::types(),
                        [](auto ct) { (void) ct; return true; },
                        [](auto ct) { return ct.local_name; },
                        [](auto ct) { return ct.icon; },
                        [this, asx, asz](auto ct, auto ti) {
                            (void) ct;
                            summon_boat(
                                *this->world, asx, asz, this->toasts,
                                (Boat::Type) ti
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

}