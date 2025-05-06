
#include "carriage.hpp"
#include "../human.hpp"
#include "../ui_const.hpp"

namespace houseofatmos::world {

    bool CarriageNetwork::is_passable(NodeId node) {
        auto [x, z] = node;
        bool has_path = this->terrain->path_at((i64) x, (i64) z)
            && this->terrain->building_at((i64) x, (i64) z) == nullptr;
        bool has_bridge = this->terrain->bridge_at((i64) x, (i64) z) != nullptr;
        return has_path || has_bridge;
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
                    0.0 // 0 degrees
                }
            },
            0.5, // wheel radius
            100, // capacity
            4.0, // speed
            1000, // cost
            Player::Rideable(
                Vec<3>(0.0, 0.6, -1.75), // position offset
                pi, // rotation offset - 180 degrees
                (u64) human::Animation::Sit // animation
            ),
            std::nullopt // does not carry passengers
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
                    0.0 // 0 degrees
                }
            },
            0.5, // wheel radius
            0, // capacity
            5.0, // speed
            1500, // cost
            Player::Rideable(
                Vec<3>(0.0, 1.25, -1.2), // position offset
                0.0, // rotation offset
                (u64) human::Animation::Sit // animation
            ),
            7.5 // carries passengers 7.5 tiles around each stop (radius)
        }
    };

    const std::vector<Carriage::CarriageTypeInfo>& Carriage::carriage_types() {
        return carriage_infos;
    }


    Carriage::Carriage(
        CarriageType type, Vec<3> position, StatefulRNG& rng, 
        const Settings& settings
    ): TileAgent<CarriageNetwork>(position) {
        this->type = type;
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
    ): TileAgent<CarriageNetwork>(serialized.agent, buffer) {
        this->type = serialized.type;
        buffer.copy_into(serialized.horses, this->horses);
        this->speaker.volume = settings.sfx_volume;
    }

    Carriage::Serialized Carriage::serialize(engine::Arena& buffer) const {
        return Serialized(
            TileAgent<CarriageNetwork>::serialize(buffer), // this is a non-static
            this->type,
            buffer.alloc(this->horses)
        );
    }

    void Carriage::update_rideable(Player& player, Interactables& interactables) {
        CarriageTypeInfo carriage_info = Carriage::carriage_types()
            .at((size_t) this->type);
        f64 yaw;
        Mat<4> inst = this->build_transform(nullptr, &yaw);
        Vec<3> offset = carriage_info.rideable.position 
            + carriage_info.carriage_offset;
        this->rideable.position = (inst * offset.with(1.0)).swizzle<3>("xyz");
        this->rideable.angle = yaw + carriage_info.rideable.angle;
        this->rideable.animation_id = carriage_info.rideable.animation_id;
        if(this->interactable != nullptr) {
            this->interactable->pos = this->rideable.position;
        }
        bool show_interaction = player.riding != &this->rideable
            && this->interactable == nullptr;
        if(!show_interaction) { return; }
        this->interactable = interactables.create([this, p = &player]() {
            p->riding = &this->rideable;
            this->interactable = nullptr;
        });
    }

    static const f64 step_sound_period = 0.5 / 5.0;

    void Carriage::update(
        CarriageNetwork& network, engine::Scene& scene, 
        const engine::Window& window, ParticleManager* particles,
        Player& player, Interactables* interactables
    ) {
        (void) particles;
        f64 speed = Carriage::carriage_types().at((size_t) this->type).speed;
        this->move_distance(window, network, window.delta_time() * speed);
        if(interactables != nullptr) {
            this->update_rideable(player, *interactables);
        }
        this->speaker.position = this->position;
        this->speaker.update();
        bool play_state_sound = this->current_state() != this->prev_state
            && this->schedule.size() > 0;
        if(play_state_sound) {
            this->prev_state = this->current_state();
            this->speaker.play(scene.get(sound::horse));
        }
        f64 next_step_time = this->last_step_time 
            + step_sound_period * speed;
        bool play_step_sound = this->current_state() == AgentState::Travelling
            && next_step_time <= window.time()
            && !this->speaker.is_playing();
        if(play_step_sound) {
            this->speaker.play(scene.get(sound::step));
            this->last_step_time = window.time();
        } 
    }

    static const f64 alignment_points_dist = 1.5;
    static const Vec<3> model_heading = Vec<3>(0, 0, 1);

    Mat<4> Carriage::build_transform(f64* pitch_out, f64* yaw_out) const {
        auto [pitch, yaw] = Agent<CarriageNetwork>
            ::compute_heading_angles(this->current_heading(), model_heading);
        if(pitch_out != nullptr) { *pitch_out = pitch; }
        if(yaw_out != nullptr) { *yaw_out = yaw; }
        return Mat<4>::translate(this->position)
            * Mat<4>::rotate_y(yaw)
            * Mat<4>::rotate_x(pitch);
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
            Mat<4> horse_transform = this->build_transform() 
                * Mat<4>::translate(carriage_info.horse_offsets[horse_i])
                * Mat<4>::translate(carriage_info.carriage_offset);
            renderer.render(
                horse_model, std::array { horse_transform },
                &horse_animation, timestamp, 
                engine::FaceCulling::Enabled,
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
        driver.update(scene, window);
        CarriageTypeInfo carriage_info = Carriage::carriage_types()
            .at((size_t) this->type);
        for(const auto& driver_inst: carriage_info.drivers) {
            f64 yaw;
            Mat<4> driver_transform 
                = this->build_transform(nullptr, &yaw)
                * Mat<4>::translate(driver_inst.offset)
                * Mat<4>::translate(carriage_info.carriage_offset);
            driver.position = (driver_transform * Vec<4>(0, 0, 0, 1))
                .swizzle<3>("xyz");
            driver.angle = yaw + driver_inst.angle;
            driver.render(scene, window, renderer);
        }
    }

    void Carriage::render_carriage(
        Renderer& renderer, engine::Scene& scene, const engine::Window& window
    ) const {
        CarriageTypeInfo carriage_info = Carriage::carriage_types()
            .at((size_t) this->type);
        engine::Model& carriage_model = scene.get(carriage_info.model);
        Mat<4> carriage_transform = this->build_transform()
            * Mat<4>::translate(carriage_info.carriage_offset);
        const engine::Animation& carriage_animation
            = carriage_model.animation("roll");
        f64 rolled_rotations = (window.time() * carriage_info.speed)
            / (carriage_info.wheel_radius * 2 * pi);
        if(this->current_state() != AgentState::Travelling) {
            rolled_rotations = 0.0;
        }
        f64 timestamp = fmod(rolled_rotations, 1.0) 
            * carriage_animation.length();
        renderer.render(
            carriage_model, std::array { carriage_transform },
            &carriage_animation, timestamp
        );
    }

    void Carriage::render(
        Renderer& renderer, CarriageNetwork& network,
        engine::Scene& scene, const engine::Window& window
    ) {
        (void) network;
        this->render_carriage(renderer, scene, window);
        this->render_drivers(renderer, scene, window);
        this->render_horses(renderer, scene, window);
    }

}