
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

    void BoatNetwork::collect_node_points(
        std::optional<NodeId> prev, NodeId node, std::optional<NodeId> next,
        std::vector<Vec<3>>& out
    ) {
        (void) prev;
        (void) next;
        auto [x, z] = node;
        Vec<3> point = Vec<3>(x, 0, z) * this->terrain->units_per_tile()
            + Vec<3>(0, water_level, 0);
        out.push_back(point);
    }



    static std::vector<Boat::TypeInfo> type_infos = {
        /* Boat::Sail */ {
            "boat_name_sail",
            &ui_icon::sail_boat,
            engine::Model::LoadArgs(
                "res/entities/sail_boat.glb", Renderer::model_attribs,
                engine::FaceCulling::Disabled
            ),
            -1.0, // may sink up to one unit if loaded fully
            1000, // capacity
            2.5, // speed
            10000 // cost
        }
    };

    const std::vector<Boat::TypeInfo>& Boat::types() {
        return type_infos;
    }


    Boat::Boat(Type type, Vec<3> position) {
        this->type = type;
        this->position = position;
    }

    Boat::Boat(
        const Serialized& serialized, const engine::Arena& buffer,
        const Settings& settings
    ): Agent<BoatNetwork>(serialized.agent, buffer) {
        (void) settings;
        this->type = serialized.type;
    }

    Boat::Serialized Boat::serialize(engine::Arena& buffer) const {
        return Serialized(
            Agent<BoatNetwork>::serialize(buffer), // this is a non-static
            this->type
        );
    }

    static const f64 alignment_points_dist = 2.5;
    static const Vec<3> model_heading = Vec<3>(0, 0, 1);

    Mat<4> Boat::build_transform(Vec<3>* position_out, f64* yaw_out) {
        TypeInfo boat_info = Boat::types().at((size_t) this->type);
        Vec<3> back = this->current_path()
            .after(this->current_path_dist() - alignment_points_dist).first;
        Vec<3> front = this->current_path()
            .after(this->current_path_dist() + alignment_points_dist).first;
        f64 storage_level = (f64) this->stored_item_count() 
            / (f64) this->item_storage_capacity();
        f64 elev = water_level + storage_level * boat_info.weight_height_factor;
        Vec<3> position = (front - back) / 2 + back + Vec<3>(0, elev, 0);
        Vec<3> heading = (front - back).normalized();
        auto [pitch, yaw] = Agent<BoatNetwork>
            ::compute_heading_angles(heading, model_heading);
        if(position_out != nullptr) { *position_out = position; }
        if(yaw_out != nullptr) { *yaw_out = yaw; }
        return Mat<4>::translate(position)
            * Mat<4>::rotate_y(yaw);
    }

    void Boat::render(
        Renderer& renderer, BoatNetwork& network,
        engine::Scene& scene, const engine::Window& window
    ) {
        (void) network;
        (void) window;
        TypeInfo boat_info = Boat::types().at((size_t) this->type);
        engine::Model& model = scene.get(boat_info.model);
        Mat<4> transform = this->build_transform();
        renderer.render(model, std::array { transform });
    }

}