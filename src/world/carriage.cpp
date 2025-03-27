
#include "carriage.hpp"
#include "../human.hpp"

namespace houseofatmos::world {

    bool CarriageNetwork::is_passable(NodeId node) const {
        auto [x, z] = node;
        bool has_path = this->terrain->path_at((i64) x, (i64) z)
            && this->terrain->building_at((i64) x, (i64) z) == nullptr;
        bool has_bridge = this->terrain->bridge_at((i64) x, (i64) z);
        return has_path || has_bridge;
    }

    static inline const u64 cost_ortho = 10;
    static inline const u64 cost_daigo = 14;

    void CarriageNetwork::collect_next_nodes(
        std::optional<NodeId> prev, NodeId node, 
        std::vector<std::pair<NodeId, u64>>& out
    ) {
        (void) prev;
        auto [x, z] = node;
        u64 left = x > 0? x - 1 : 0;
        u64 right = std::min(x + 1, this->terrain->width_in_tiles() - 1);
        u64 top = z > 0? z - 1 : 0;
        u64 bottom = std::min(z + 1, this->terrain->height_in_tiles() - 1);
        for(u64 nx = left; nx <= right; nx += 1) {
            for(u64 nz = top; nz <= bottom; nz += 1) {
                NodeId neighbor = { nx, nz };
                if(nx == x && nz == z) { continue; }
                if(!this->is_passable(neighbor)) { continue; }
                bool is_diagonal = nx != x && nz != z;
                u64 cost = is_diagonal? cost_daigo : cost_ortho;
                out.push_back({ neighbor, cost });
            }
        }
    }

    u64 CarriageNetwork::node_target_dist(NodeId node, ComplexId target) {
        auto [nx, nz] = node;
        // find target building start coords
        const Complex& complex = this->complexes->get(target);
        auto [bsx, bsz] = complex.closest_member_to(nx, nz);
        // find target building end coordinates
        // (end in this case meaning the last tile still inside the building)
        const Building* building = this->terrain
            ->building_at((i64) bsx, (i64) bsz);
        const Building::TypeInfo& building_type = Building::types()
            .at((size_t) building->type);
        u64 bex = bsx + building_type.width - 1;
        u64 bez = bsz + building_type.height - 1;
        // distance on each individual axis
        u64 dx = nx < bsx? bsx - nx // left of building
            : nx > bex? nx - bex    // right of building
            : 0;                    // on X axis inside building
        u64 dz = nz < bsz? bsz - nz // top of building
            : nz > bez? nz - bez    // below building
            : 0;                    // on Z axis inside building
        return dx + dz; // manhattan distance
    }

    bool CarriageNetwork::node_at_target(NodeId node, ComplexId target) {
        return this->node_target_dist(node, target) <= 1;
    }

    static inline const f64 max_path_pos_var = 0.05;

    void CarriageNetwork::collect_node_points(
        std::optional<NodeId> prev, NodeId node, std::optional<NodeId> next,
        std::vector<Vec<3>>& out
    ) {
        (void) prev;
        (void) next;
        auto [x, z] = node;
        Vec<3> o = Vec<3>(this->rng.next_f64(), 0, this->rng.next_f64());
        o = (o * 2.0 - Vec<3>(1.0, 0.0, 1.0)) * max_path_pos_var;
        Vec<3> point = Vec<3>(x, 0, z) * this->terrain->units_per_tile()
            + (o + Vec<3>(0.5, 0, 0.5)) * this->terrain->units_per_tile();
        point.y() = this->terrain->elevation_at(point);
        out.push_back(point);
    }

    std::optional<CarriageNetwork::NodeId> CarriageNetwork::closest_node_to(
        const Vec<3>& position
    ) {
        Vec<3> tile = position / this->terrain->units_per_tile();
        u64 x = (u64) std::max(tile.x(), 0.0);
        x = std::min(x, this->terrain->width_in_tiles());
        u64 z = (u64) std::max(tile.z(), 0.0);
        z = std::min(z, this->terrain->height_in_tiles());
        return CarriageNetwork::NodeId(x, z);
    }



    static std::vector<Carriage::HorseTypeInfo> horse_infos = {
        // The 'true' in each texture loader is there to flip the loaded
        // texture vertically - this is needed since GLTF uses them flipped
        /* White */ {
            engine::Texture::LoadArgs(
                "res/entities/horse_white.png", 
                engine::Texture::vertical_mirror
            )
        },
        /* WhiteSpotted */ {
            engine::Texture::LoadArgs(
                "res/entities/horse_white_spotted.png",
                engine::Texture::vertical_mirror
            )
        },
        /* Brown */ {
            engine::Texture::LoadArgs(
                "res/entities/horse_brown.png",
                engine::Texture::vertical_mirror
            )
        },
        /* BrownSpotted */ {
            engine::Texture::LoadArgs(
                "res/entities/horse_brown_spotted.png",
                engine::Texture::vertical_mirror
            )
        },
        /* BlackSpotted */ {
            engine::Texture::LoadArgs(
                "res/entities/horse_black_spotted.png",
                engine::Texture::vertical_mirror
            )
        }
    };

    const std::vector<Carriage::HorseTypeInfo>& Carriage::horse_types() {
        return horse_infos;
    }


    static std::vector<Carriage::CarriageTypeInfo> carriage_infos = {
        /* Round */ {
            "carriage_name_round",
            &ui_icon::round_carriage,
            engine::Model::LoadArgs(
                "res/entities/round_carriage.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            ),
            Vec<3>(0.0, 0.0, -1.5),
            { // horses
                Vec<3>(0.0, 0.0, 4.5)
            },
            {
                (Carriage::CarriageTypeInfo::Driver) {
                    Vec<3>(0.0, 1.2, 2.35),
                    pi / 2.0 // 90 degrees
                }
            },
            0.5, // wheel radius
            100, // capacity
            4.0, // speed
            1000 // cost
        },
        /* Passenger */ {
            "carriage_name_passenger",
            &ui_icon::passenger_carriage,
            engine::Model::LoadArgs(
                "res/entities/passenger_carriage.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            ),
            Vec<3>(0.0, 0.0, -1.5),
            { // horses
                Vec<3>(0.0, 0.0, 4.5)
            },
            {
                (Carriage::CarriageTypeInfo::Driver) {
                    Vec<3>(0.0, 1.2, 2.35),
                    pi / 2.0 // 90 degrees
                }
            },
            0.5, // wheel radius
            0, // capacity
            5.0, // speed
            1500 // cost
        }
    };

    const std::vector<Carriage::CarriageTypeInfo>& Carriage::carriage_types() {
        return carriage_infos;
    }


    Carriage::Carriage(
        CarriageType type, Vec<3> position, StatefulRNG& rng, 
        const Settings& settings
    ) {
        this->type = type;
        this->position = position;
        CarriageTypeInfo carriage_info = Carriage::carriage_types()
            .at((size_t) this->type);
        size_t horse_count = carriage_info.horse_offsets.size();
        this->horses = std::vector<HorseType>(horse_count);
        for(size_t horse_i = 0; horse_i < horse_count; horse_i += 1) {
            u64 h_type = rng.next_u64() % Carriage::horse_types().size();
            this->horses.at(horse_i) = (HorseType) h_type;
        }
        this->speaker.volume = settings.sfx_volume;
    }

    Carriage::Carriage(
        const Serialized& serialized, const engine::Arena& buffer,
        const Settings& settings
    ): Agent<CarriageNetwork>(serialized.agent, buffer) {
        this->type = serialized.type;
        buffer.copy_array_at_into(
            serialized.horses_offset, serialized.horses_count, this->horses
        );
        this->speaker.volume = settings.sfx_volume;
    }

    Carriage::Serialized Carriage::serialize(engine::Arena& buffer) const {
        return Serialized(
            Agent<CarriageNetwork>::serialize(buffer), // this is a non-static
            this->type,
            this->horses.size(), buffer.alloc_array(this->horses)
        );
    }

    static const f64 step_sound_period = 0.5 / 5.0;

    void Carriage::update(
        CarriageNetwork& network, engine::Scene& scene, 
        const engine::Window& window, ParticleManager* particles
    ) {
        (void) particles;
        this->speaker.position = this->position;
        this->speaker.update();
        bool play_state_sound = this->current_state() != this->prev_state
            && this->schedule.size() > 0;
        if(play_state_sound) {
            this->prev_state = this->current_state();
            this->speaker.play(scene.get(sound::horse));
        }
        f64 next_step_time = this->last_step_time 
            + step_sound_period * this->current_speed(network);
        bool play_step_sound = this->current_state() == AgentState::Travelling
            && next_step_time <= window.time()
            && !this->speaker.is_playing();
        if(play_step_sound) {
            this->speaker.play(scene.get(sound::step));
            this->last_step_time = window.time();
        } 
    }

    static const f64 alignment_points_dist = 1.5;

    Vec<3> Carriage::find_heading() const {
        Vec<3> back = this->current_path()
            .after(this->current_path_dist() - alignment_points_dist).first;
        Vec<3> front = this->current_path()
            .after(this->current_path_dist() + alignment_points_dist).first;
        return (front - back).normalized();
    }

    void Carriage::render_horses(
        Renderer& renderer, engine::Scene& scene, const engine::Window& window
    ) const {
        engine::Model& horse_model = scene.get(Carriage::horse_model);
        CarriageTypeInfo carriage_info = Carriage::carriage_types()
            .at((size_t) this->type);
        bool is_moving = this->current_state() == AgentState::Travelling;
        for(size_t horse_i = 0; horse_i < this->horses.size(); horse_i += 1) {
            HorseType horse_type = this->horses.at(horse_i);
            HorseTypeInfo horse_info = Carriage::horse_types()
                .at((size_t) horse_type);
            const engine::Texture& horse_texture
                = scene.get(horse_info.texture);
            const engine::Animation& horse_animation = horse_model
                .animation(is_moving? "walk" : "idle");
            f64 timestamp = fmod(
                window.time() * (is_moving? 2.5 : 0.5), 
                horse_animation.length()
            );
            Mat<4> horse_transform
                = Mat<4>::translate(this->position)
                * Mat<4>::rotate_y(this->yaw)
                * Mat<4>::rotate_x(this->pitch)
                * Mat<4>::translate(carriage_info.horse_offsets[horse_i])
                * Mat<4>::translate(carriage_info.carriage_offset);
            renderer.render(
                horse_model, std::array { horse_transform },
                &horse_animation, timestamp, 
                engine::FaceCulling::Enabled,
                engine::Rendering::Surfaces, 
                engine::DepthTesting::Enabled,
                &horse_texture
            );
        }
    }

    static Character driver = Character(
        &human::male, &human::peasant_man, 
        { 0, 0, 0 }, (u64) human::Animation::Sit
    );

    void Carriage::render_drivers(
        Renderer& renderer, engine::Scene& scene, const engine::Window& window
    ) const {
        CarriageTypeInfo carriage_info = Carriage::carriage_types()
            .at((size_t) this->type);
        for(const auto& driver_inst: carriage_info.drivers) {
            Mat<4> driver_transform = Mat<4>::translate(this->position)
                * Mat<4>::rotate_y(this->yaw)
                * Mat<4>::rotate_x(this->pitch)
                * Mat<4>::translate(driver_inst.offset)
                * Mat<4>::translate(carriage_info.carriage_offset);
            driver.position = (driver_transform * Vec<4>(0, 0, 0, 1))
                .swizzle<3>("xyz");
            f64 angle = this->yaw + driver_inst.angle;
            driver.face_in_direction(Vec<3>(cos(angle), 0, sin(angle)));
            driver.render(scene, window, renderer);
        }
    }

    void Carriage::render_carriage(
        Renderer& renderer, engine::Scene& scene
    ) const {
        CarriageTypeInfo carriage_info = Carriage::carriage_types()
            .at((size_t) this->type);
        engine::Model& carriage_model = scene.get(carriage_info.model);
        Mat<4> carriage_transform 
            = Mat<4>::translate(this->position)
            * Mat<4>::rotate_y(this->yaw)
            * Mat<4>::rotate_x(this->pitch)
            * Mat<4>::translate(carriage_info.carriage_offset);
        const engine::Animation& carriage_animation
            = carriage_model.animation("roll");
        f64 rolled_rotations = this->current_path_dist()
            / (carriage_info.wheel_radius * 2 * pi);
        f64 timestamp = fmod(rolled_rotations, 1.0) 
            * carriage_animation.length();
        renderer.render(
            carriage_model, std::array { carriage_transform },
            &carriage_animation, timestamp
        );
    }

    static const Vec<3> model_heading = Vec<3>(0, 0, 1);

    void Carriage::render(
        Renderer& renderer, CarriageNetwork& network,
        engine::Scene& scene, const engine::Window& window
    ) {
        (void) network;
        if(this->current_state() == AgentState::Travelling) {
            Vec<3> heading = this->find_heading();
            auto [n_pitch, n_yaw] = Agent<CarriageNetwork>
                ::compute_heading_angles(heading, model_heading);
            this->pitch = n_pitch;
            this->yaw = n_yaw;
        }
        this->render_carriage(renderer, scene);
        this->render_drivers(renderer, scene, window);
        this->render_horses(renderer, scene, window);
    }

}