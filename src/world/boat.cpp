
#include "boat.hpp"
#include "../ui_const.hpp"

namespace houseofatmos::world {

    static const f64 water_level = -0.5;
    static inline const f64 min_allowed_bridge_height = 10.0;

    bool BoatNetwork::is_passable(NodeId node) {
        auto [x, z] = node;
        bool xl = x == 0;
        bool zl = z == 0;
        bool xh = x + 1 >= this->terrain->width_in_tiles();
        bool zh = z + 1 >= this->terrain->height_in_tiles();
        bool has_water
            =  (xl || zl || this->terrain->elevation_at(x - 1, z - 1) < 0)
            && (      zl || this->terrain->elevation_at(x,     z - 1) < 0)
            && (xh || zl || this->terrain->elevation_at(x + 1, z - 1) < 0)
            && (xl ||       this->terrain->elevation_at(x - 1, z    ) < 0)
            && (            this->terrain->elevation_at(x,     z    ) < 0)
            && (xh ||       this->terrain->elevation_at(x + 1, z    ) < 0)
            && (xl || zh || this->terrain->elevation_at(x - 1, z + 1) < 0)
            && (      zh || this->terrain->elevation_at(x,     z + 1) < 0)
            && (xh || zh || this->terrain->elevation_at(x + 1, z + 1) < 0);
        if(!has_water) { return false; }
        const Bridge* bridge = this->terrain->bridge_at(x, z);
        if(bridge != nullptr) {
            const Bridge::TypeInfo& bridge_info = Bridge::types()
                .at((size_t) bridge->type);
            if(!bridge_info.allows_boat_passage) { return false; }
            f64 bridge_clearance = (f64) bridge->floor_y 
                - bridge_info.min_height;
            if(bridge_clearance < min_allowed_bridge_height) { return false; }
        }
        return true;
    }



    static std::vector<Boat::TypeInfo> type_infos = {
        /* Boat::Sail */ {
            "boat_name_sail",
            &ui_icon::sail_boat,
            engine::Model::LoadArgs(
                "res/entities/sail_boat.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            ),
            {
                (Boat::TypeInfo::CrewMember) {
                    Vec<3>(0.0, 2.5, -4.4), // position offset
                    0.0, // rotation offset
                    (u64) human::Animation::Stand // animation
                },
                (Boat::TypeInfo::CrewMember) {
                    Vec<3>(-1.5, 2.25, 0.0), // position offset
                    pi / 2, // rotation offset
                    (u64) human::Animation::Stand // animation
                }
            },
            -1.0, // may sink up to one unit if loaded fully
            1000, // capacity
            2.5, // speed
            10000 // cost
        }
    };

    const std::vector<Boat::TypeInfo>& Boat::types() {
        return type_infos;
    }


    Boat::Boat(Type type, Vec<3> position): TileAgent<BoatNetwork>(position) {
        this->type = type;
    }

    Boat::Boat(
        const Serialized& serialized, const engine::Arena& buffer,
        const Settings& settings
    ): TileAgent<BoatNetwork>(serialized.agent, buffer) {
        (void) settings;
        this->type = serialized.type;
    }

    Boat::Serialized Boat::serialize(engine::Arena& buffer) const {
        return Serialized(
            TileAgent<BoatNetwork>::serialize(buffer), // this is a non-static
            this->type
        );
    }

    static const f64 alignment_points_dist = 2.5;
    static const Vec<3> model_heading = Vec<3>(0, 0, 1);

    Mat<4> Boat::build_transform(f64* yaw_out) {
        auto [pitch, yaw] = Agent<BoatNetwork>
            ::compute_heading_angles(this->current_heading(), model_heading);
        if(yaw_out != nullptr) { *yaw_out = yaw; }
        return Mat<4>::translate(this->position)
            * Mat<4>::rotate_y(yaw);
    }

    static Character crew_member = Character(
        &human::male, &human::peasant_man, 
        { 0, 0, 0 }, (u64) human::Animation::Sit
    );

    void Boat::update(
        BoatNetwork& network, engine::Scene& scene, 
        const engine::Window& window, ParticleManager* particles,
        Player& player, Interactables* interactables
    ) {
        (void) scene;
        (void) particles;
        (void) player;
        (void) interactables;
        TypeInfo boat_info = Boat::types().at((size_t) this->type);
        this->move_distance(
            window, network, window.delta_time() * boat_info.speed
        );
        u64 total_item_count = 0;
        for(const auto& stack: this->items) {
            total_item_count += stack.second;
        }
        f64 storage_level = (f64) total_item_count
            / (f64) this->item_storage_capacity();
        this->position.y() = water_level 
            + storage_level * boat_info.weight_height_factor;
    }

    void Boat::render(
        Renderer& renderer, BoatNetwork& network,
        engine::Scene& scene, const engine::Window& window
    ) {
        (void) network;
        (void) window;
        TypeInfo boat_info = Boat::types().at((size_t) this->type);
        engine::Model& model = scene.get(boat_info.model);
        f64 yaw;
        Mat<4> transform = this->build_transform(&yaw);
        renderer.render(model, std::array { transform });
        crew_member.update(scene, window);
        for(const Boat::TypeInfo::CrewMember& m: boat_info.crew_members) {
            crew_member.position = (transform * m.offset.with(1))
                .swizzle<3>("xyz");
            crew_member.angle = yaw + m.angle;
            crew_member.action = Character::Action(m.animation_id, INFINITY);
            crew_member.render(scene, window, renderer);
        }
    }

}