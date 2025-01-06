
#include "carriage.hpp"

namespace houseofatmos::outside {

    Carriage::Carriage(
        CarriageType type, Vec<3> position,
        StatefulRNG rng
    ) {
        this->type = type;
        CarriageTypeInfo type_info = Carriage::carriage_types.at((size_t) type);
        this->horses = std::vector<HorseType>(type_info.horse_offsets.size());
        for(size_t horse_i = 0; horse_i < this->horses.size(); horse_i += 1) {
            this->horses.at(horse_i) = (HorseType) (
                rng.next_u64() % Carriage::horse_types.size()
            );
        }
        this->curr_target_i = 0;
        this->position = position;
        this->yaw = 0.0;
        this->pitch = 0.0;
        this->travelled_dist = 0.0;
    }

    Carriage::Carriage(
        const Serialized& serialized, const engine::Arena& buffer
    ) {
        this->type = serialized.type;
        buffer.copy_array_at_into(
            serialized.horses_offset, serialized.horses_count,
            this->horses
        );
        buffer.copy_array_at_into(
            serialized.targets_offset, serialized.targets_count,
            this->targets
        );
        this->curr_target_i = serialized.curr_target_i;
        this->position = serialized.position;
        this->yaw = 0.0;
        this->pitch = 0.0;
        this->travelled_dist = 0.0;
    }


    static const Vec<3> model_heading = Vec<3>(0, 0, -1);

    void Carriage::render(
        Renderer& renderer, engine::Scene& scene, 
        const engine::Window& window
    ) {
        bool moved = this->position != this->last_position;
        Vec<3> pos_diff = this->position - this->last_position;
        if(moved) {
            Vec<3> heading = pos_diff.normalized();
            f64 yaw_cross = model_heading.x() * heading.z()
                - model_heading.z() * heading.x();
            this->yaw = atan2(yaw_cross, model_heading.dot(heading));
            Vec<3> vert_heading = Vec<3>(
                model_heading.x(),
                heading.y(),
                model_heading.z()
            );
            f64 pitch_cross = model_heading.y() * vert_heading.z()
                - model_heading.z() * vert_heading.y();
            this->pitch = atan2(pitch_cross, model_heading.dot(vert_heading));
        }
        this->last_position = this->position;
        CarriageTypeInfo carriage_info = Carriage::carriage_types
            .at((size_t) this->type);
        // render horses
        engine::Model& horse_model = scene
            .get<engine::Model>(Carriage::horse_model);
        for(size_t horse_i = 0; horse_i < this->horses.size(); horse_i += 1) {
            HorseType horse_type = this->horses.at(horse_i);
            HorseTypeInfo horse_info = Carriage::horse_types
                .at((size_t) horse_type);
            const engine::Texture& horse_texture = scene
                .get<engine::Texture>(horse_info.texture);
            const engine::Animation& horse_animation = moved
                ? horse_model.animation("walk")
                : horse_model.animation("idle");
            f64 timestamp = fmod(
                window.time() * (moved? 2.5 : 0.5), horse_animation.length()
            );
            Mat<4> horse_transform
                = Mat<4>::translate(this->position)
                * Mat<4>::rotate_y(this->yaw)
                * Mat<4>::rotate_x(this->pitch)
                * Mat<4>::translate(carriage_info.horse_offsets.at(horse_i))
                * Mat<4>::translate(carriage_info.carriage_offset);
            renderer.render(
                horse_model, std::array { horse_transform },
                horse_animation, timestamp, false, &horse_texture
            );
        }
        // render carriage
        engine::Model& carriage_model = scene
            .get<engine::Model>(carriage_info.model);
        Mat<4> carriage_transform 
            = Mat<4>::translate(this->position)
            * Mat<4>::rotate_y(this->yaw)
            * Mat<4>::rotate_x(this->pitch)
            * Mat<4>::translate(carriage_info.carriage_offset);
        const engine::Animation& carriage_animation
            = carriage_model.animation("roll");
        f64 rolled_rotations = pos_diff.len()
            / (carriage_info.wheel_radius * 2 * pi);
        this->wheel_timer = fmod(this->wheel_timer + rolled_rotations, 1.0); 
        renderer.render(
            carriage_model, std::array { carriage_transform },
            carriage_animation, this->wheel_timer
        );
    }


    Carriage::Serialized Carriage::serialize(engine::Arena& buffer) const {
        return (Serialized) {
            this->type,
            this->horses.size(), buffer.alloc_array(this->horses),
            this->targets.size(), buffer.alloc_array(this->targets),
            this->curr_target_i,
            this->position
        };
    }

}