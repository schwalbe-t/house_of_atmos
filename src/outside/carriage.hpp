
#pragma once

#include <engine/arena.hpp>
#include <engine/math.hpp>
#include <engine/model.hpp>
#include <engine/rng.hpp>
#include "../renderer.hpp"
#include "complex.hpp"
#include "terrain.hpp"

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


        enum struct State {
            Travelling, Loading, Lost
        };

        struct Serialized {
            CarriageType type;
            u64 horses_count, horses_offset;
            u64 targets_count, targets_offset;
            u64 items_count, items_offset;
            u64 curr_target_i;
            State state;
            Vec<3> position;
        };

        enum TargetAction {
            LoadFixed, LoadPercentage,
            PutFixed, PutPercentage
        };

        struct Target {
            ComplexId complex;
            TargetAction action;
            union {
                u32 fixed;
                f32 percentage;
            } amount;
            Item item;
        };


        private:
        CarriageType type;
        std::vector<HorseType> horses;
        std::unordered_map<Item, u64> items;
        u64 curr_target_i;
        State state;

        f64 yaw, pitch;
        std::vector<Vec<3>> curr_path;
        f64 travelled_dist;
        f64 load_timer;
        bool moving;


        public:
        std::vector<Target> targets;
        Vec<3> position;

        Carriage(
            CarriageType type, Vec<3> position,
            StatefulRNG rng = StatefulRNG()
        );
        Carriage(const Serialized& serialized, const engine::Arena& buffer);

        void clear_path() { this->curr_path.clear(); }
        bool has_path() const { return this->curr_path.size() > 0; }
        void set_path(std::vector<Vec<3>>& path) {
            this->curr_path.assign(path.begin(), path.end());
            this->travelled_dist = 0.0;
            if(this->state == State::Lost) {
                this->state = State::Travelling;
            }
        }

        bool is_lost() const { return this->state == State::Lost; }
        void make_lost() { this->state = State::Lost; }

        u64 stored_count(Item item) const {
            auto count = this->items.find(item);
            if(count == this->items.end()) { return 0; }
            return count->second;
        }
        void add_stored(Item item, u64 amount) { this->items[item] += amount; }
        void remove_stored(Item item, u64 amount) { this->items[item] -= amount; }
        void set_stored(Item item, u64 amount) { this->items[item] = amount; }
        const std::unordered_map<Item, u64>& stored_items() const {
            return this->items;
        }

        const Target& target() const {
            return this->targets[this->curr_target_i]; 
        }

        void update(
            const engine::Window& window,
            ComplexBank& complexes, const Terrain& terrain
        );

        void render(
            Renderer& renderer, engine::Scene& scene, 
            const engine::Window& window
        );

        Serialized serialize(engine::Arena& buffer) const;

    };


    struct CarriageManager {

        struct Serialized {
            u64 carriage_count, carriage_offset;
        };


        private:
        std::vector<u8> obstacle_tiles;

        void fill_obstacle_data(const Terrain& terrain);


        public:
        std::vector<Carriage> carriages;

        CarriageManager() {}
        CarriageManager(const Terrain& terrain);
        CarriageManager(
            const Serialized& serialized, const engine::Arena& buffer,
            const Terrain& terrain
        );

        std::optional<std::vector<Vec<3>>> find_path_to(
            const Vec<3>& start,
            const Complex& target, const Terrain& terrain
        );

        void find_carriage_path(
            Carriage& carriage, 
            const ComplexBank& complexes, const Terrain& terrain
        );
        void refind_all_paths(
            const ComplexBank& complexes, const Terrain& terrain
        );
        
        void update_all(
            const engine::Window& window, 
            ComplexBank& complexes, const Terrain& terrain   
        );

        void render_all(
            Renderer& renderer, engine::Scene& scene, 
            const engine::Window& window
        );

        Serialized serialize(engine::Arena& buffer) const;

    };

}