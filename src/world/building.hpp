
#pragma once

#include "../interior/interiors.hpp"
#include "../renderer.hpp"
#include "../collider.hpp"
#include "../ui_const.hpp"
#include "../particles.hpp"
#include "complex_id.hpp"

namespace houseofatmos::world {

    using namespace houseofatmos;

    struct Building {

        struct LinkedInterior {
            Vec<3> interactable_offset;
            const interior::Interior& interior;
        };

        struct TypeInfo {
            static inline const bool remove_terrain = false;
            static inline const bool keep_terrain = true;

            static inline const bool indestructible = false;
            static inline const bool allow_destruction = true;

            std::string_view local_name;
            const ui::Background* icon;
            bool terrain_under_building; // false = remove terrain under building
            engine::Model::LoadArgs model;
            std::optional<std::string_view> animation;
            f64 animation_speed;
            std::vector<RelCollider> colliders; // in game units
            u8 width, height; // in tiles
            std::optional<LinkedInterior> interior;
            u64 cost; // in coins
            u64 capacity;
            bool destructible;
            u64 workers;

            std::optional<std::function<ParticleSpawner (Vec<3>, StatefulRNG&)>> 
                particle_spawner;

            void render_buildings(
                const engine::Window& window, engine::Scene& scene,
                Renderer& renderer,
                std::span<const Mat<4>> instances,
                const engine::Texture* override_texture = nullptr,
                engine::DepthTesting depth_testing 
                    = engine::DepthTesting::Enabled
            ) const {
                engine::Model& model = scene.get(this->model);
                if(!this->animation.has_value()) {
                    renderer.render(
                        model, instances, 
                        nullptr, 0.0,
                        engine::FaceCulling::Enabled,
                        depth_testing,
                        override_texture
                    );
                    return;
                }
                const engine::Animation& anim = model
                    .animation(std::string(*this->animation));
                f64 timestamp = fmod(
                    window.time() * this->animation_speed, anim.length()
                );
                renderer.render(
                    model, instances, 
                    &anim, timestamp, 
                    engine::FaceCulling::Enabled,
                    depth_testing, 
                    override_texture
                );
            }
        };

        static const std::vector<TypeInfo>& types();

        static void load_models(engine::Scene& scene) {
            for(const TypeInfo& type: Building::types()) {
                scene.load(type.model);
            }
        }


        enum Type {
            Farmland,
            Mineshaft,
            Windmill,
            Factory,
            House,
            Stable,
            Plaza,
            Mansion,
            Pasture,
            TreeFarm,
            TrainDepot,
            ShipYard,
            Storage,
            CommissaryWorks,
            ManufacturingWorks,
            ClothWorks,
            MetallurgicalWorks
        };

        enum struct WorkerState {
            Working,
            Shortage,
            Unreachable
        };

        struct Serialized {
            Type type;
            u8 x, z; // in tiles relative to chunk origin
            u8 has_complex;
            ComplexId complex;
        };

        Type type;
        u8 x, z; // in tiles relative to chunk origin
        std::optional<ComplexId> complex = std::nullopt;
        WorkerState workers;

        Building(
            Type type, u8 x, u8 z, 
            std::optional<ComplexId> complex = std::nullopt
        ): type(type), x(x), z(z), complex(complex), 
            workers(WorkerState::Working) {}
        Building(const Serialized& serialized) {
            this->type = serialized.type;
            this->x = serialized.x;
            this->z = serialized.z;
            this->complex = serialized.has_complex
                ? std::optional<ComplexId>(serialized.complex) : std::nullopt;
            this->workers = WorkerState::Working;
        }

        Serialized serialize() const {
            return {
                this->type,
                this->x, this->z,
                this->complex.has_value(),
                this->complex.has_value()? *this->complex : ComplexId(0)
            };
        }

        const TypeInfo& get_type_info() const {
            return Building::types().at((size_t) this->type);
        }

    };

}