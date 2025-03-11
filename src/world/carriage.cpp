
#include "carriage.hpp"

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
        NodeId node, std::vector<std::pair<NodeId, u64>>& out
    ) {
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
                u64 cost = nx == is_diagonal? cost_daigo : cost_ortho;
                out.push_back({ neighbor, cost });
            }
        }
    }

    u64 CarriageNetwork::node_target_dist(NodeId node, ComplexId target) {
        auto [nx, nz] = node;
        const Complex& complex = this->complexes->get(target);
        auto [cx, cz] = complex.closest_member_to(node.first, node.second);
        u64 dx = (u64) std::abs((i64) nx - (i64) cx);
        u64 dz = (u64) std::abs((i64) nz - (i64) cz);
        u64 diagonal = std::min(dx, dz);
        return (dx - diagonal) * cost_ortho + (dz - diagonal) * cost_ortho 
            + diagonal * cost_daigo;
    }

    static const std::vector<std::pair<i64, i64>> valid_target_offsets = {
        { +1,  0 },
        { -1,  0 },
        {  0, +1 },
        {  0, -1 }
    };

    bool CarriageNetwork::node_at_target(NodeId node, ComplexId target) {
        for(auto [ox, oz]: valid_target_offsets) {
            const Building* building = this->terrain
                ->building_at((i64) node.first + ox, (i64) node.second + oz);
            bool is_valid = building != nullptr
                && building->complex.has_value()
                && building->complex->index == target.index;
            if(is_valid) { return true; }
        }
        return false;
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

    CarriageNetwork::NodeId CarriageNetwork::closest_node_to(
        const Vec<3>& position
    ) {
        Vec<3> tile = position / this->terrain->units_per_tile();
        u64 x = (u64) std::max(tile.x(), 0.0);
        x = std::min(x, this->terrain->width_in_tiles());
        u64 z = (u64) std::max(tile.z(), 0.0);
        z = std::min(z, this->terrain->height_in_tiles());
        return { x, z };
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
            4.0  // speed
        }
    };

    const std::vector<Carriage::CarriageTypeInfo>& Carriage::carriage_types() {
        return carriage_infos;
    }


    Carriage::Carriage(CarriageType type, Vec<3> position) {
        this->type = type;
        this->position = position;
    }

    Carriage::Carriage(
        const Serialized& serialized, const engine::Arena& buffer
    ) {
        this->type = serialized.type;
        buffer.copy_array_at_into(
            serialized.horses_offset, serialized.horses_count, this->horses
        );
        buffer.copy_array_at_into(
            serialized.stop_offset, serialized.stop_count, this->schedule
        );
        this->stop_i = serialized.stop_i;
        this->position = serialized.position;
        buffer.copy_map_at_into(
            serialized.items_offset, serialized.items_count, this->items
        );
    }

    Carriage::Serialized Carriage::serialize(engine::Arena& buffer) const {
        return Serialized(
            this->type,
            this->horses.size(), buffer.alloc_array(this->horses),
            this->schedule.size(), buffer.alloc_array(this->schedule),
            this->stop_i,
            this->position,
            this->items.size(), buffer.alloc_map(this->items)
        );
    }

}