
#pragma once

#include <engine/arena.hpp>
#include <engine/math.hpp>
#include <engine/model.hpp>
#include <engine/rng.hpp>
#include "../renderer.hpp"
#include "complex.hpp"

namespace houseofatmos::outside {

    using namespace houseofatmos;
    using namespace houseofatmos::engine::math;


    struct Carriage {

        static const inline engine::Model::LoadArgs horse_model = {
            "res/entities/horse.glb", Renderer::model_attribs
        };


        struct HorseTypeInfo {
            engine::Texture::LoadArgs texture;
        };

        static const inline std::vector<HorseTypeInfo> horse_types = {
            /* White */ {
                (engine::Texture::LoadArgs) { "res/entities/horse.png" }
            }
        };

        enum struct HorseType {
            White = 0
        };


        struct CarriageTypeInfo {
            engine::Model::LoadArgs model;
            Vec<3> carriage_offset;
            std::vector<Vec<3>> horse_offsets;
            f64 wheel_radius;
        };

        static const inline std::vector<CarriageTypeInfo> carriage_types = {
            /* Round */ {
                (engine::Model::LoadArgs) {
                    "res/entities/round_carriage.glb", Renderer::model_attribs
                },
                Vec<3>(0.0, 0.0, 1.5),
                (std::vector<Vec<3>>) {
                    Vec<3>(0.0, 0.0, -4.5)
                },
                0.5 // wheel radius 
            }
        };

        enum CarriageType {
            Round = 0
        };


        static void load_resources(engine::Scene& scene) {
            scene.load(engine::Model::Loader(Carriage::horse_model));
            for(const HorseTypeInfo& horse_type: Carriage::horse_types) {
                scene.load(engine::Texture::Loader(horse_type.texture));
            }
            for(const CarriageTypeInfo& carriage_type: Carriage::carriage_types) {
                scene.load(engine::Model::Loader(carriage_type.model));
            }
        }


        struct Serialized {
            CarriageType type;
            u64 horses_count, horses_offset;
            u64 targets_count, targets_offset;
            u64 curr_target_i;
            Vec<3> position;
        };


        private:
        CarriageType type;
        std::vector<HorseType> horses;
        std::vector<ComplexId> targets;
        u64 curr_target_i;

        f64 yaw, pitch;
        f64 wheel_timer;
        Vec<3> last_position;
        std::vector<Vec<3>> curr_path;
        f64 travelled_dist;


        public:
        Vec<3> position;

        Carriage(
            CarriageType type, Vec<3> position,
            StatefulRNG rng = StatefulRNG()
        );
        Carriage(const Serialized& serialized, const engine::Arena& buffer);


        std::vector<ComplexId>& all_targets() {
            return this->targets;
        }
        const std::vector<ComplexId>& all_targets() const {
            return this->targets;
        }


        void render(
            Renderer& renderer, engine::Scene& scene, 
            const engine::Window& window
        );


        Serialized serialize(engine::Arena& buffer) const;

    };

}