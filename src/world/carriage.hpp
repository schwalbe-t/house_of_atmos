
#pragma once

#include <engine/arena.hpp>
#include <engine/math.hpp>
#include <engine/model.hpp>
#include <engine/rng.hpp>
#include "../renderer.hpp"
#include "../toasts.hpp"
#include "../settings.hpp"
#include "complex.hpp"
#include "terrain.hpp"

namespace houseofatmos::world {

    using namespace houseofatmos;
    using namespace houseofatmos::engine::math;


    struct Carriage {

        static const inline engine::Model::LoadArgs horse_model = {
            "res/entities/horse.glb", Renderer::model_attribs,
            engine::FaceCulling::Disabled
        };


        struct HorseTypeInfo {
            engine::Texture::LoadArgs texture;
        };

        static const std::vector<HorseTypeInfo>& horse_types();

        enum struct HorseType {
            White = 0,
            WhiteSpotted = 1,
            Brown = 2,
            BrownSpotted = 3,
            BlackSpotted = 4
        };


        struct CarriageTypeInfo {
            struct Driver {
                Vec<3> offset;
                f64 angle;
            };
            
            engine::Model::LoadArgs model;
            Vec<3> carriage_offset;
            std::vector<Vec<3>> horse_offsets;
            std::vector<Driver> drivers;
            f64 wheel_radius;
            u64 capacity;
        };

        static const std::vector<CarriageTypeInfo>& carriage_types();

        enum CarriageType {
            Round = 0
        };


        static void load_resources(engine::Scene& scene) {
            scene.load(Carriage::horse_model);
            for(const HorseTypeInfo& horse_type: Carriage::horse_types()) {
                scene.load(horse_type.texture);
            }
            for(const CarriageTypeInfo& carriage_type: Carriage::carriage_types()) {
                scene.load(carriage_type.model);
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
            Item::Type item;
        };


        private:
        engine::Speaker speaker = engine::Speaker(
            engine::Speaker::Space::World, 5.0
        );
        CarriageType type;
        std::vector<HorseType> horses;
        std::unordered_map<Item::Type, u64> items;
        u64 curr_target_i;
        State state;

        f64 yaw, pitch;
        std::vector<Vec<3>> curr_path;
        f64 travelled_dist;
        f64 load_timer;
        f64 sound_timer;
        bool moving;


        public:
        std::vector<Target> targets;
        Vec<3> position;

        Carriage(
            const Settings& settings,
            CarriageType type, Vec<3> position,
            StatefulRNG rng = StatefulRNG()
        );
        Carriage(
            const Settings& settings, 
            const Serialized& serialized, const engine::Arena& buffer
        );

        void clear_path() { this->curr_path.clear(); }
        bool has_path() const { return this->curr_path.size() > 0; }
        void set_path(std::vector<Vec<3>>& path) {
            this->curr_path.assign(path.begin(), path.end());
            this->travelled_dist = 0.0;
            if(this->state == State::Lost) {
                this->state = State::Travelling;
            }
        }

        bool is_lost() const {
            return this->state == State::Lost
                && this->target() != nullptr; 
        }
        void make_lost() { this->state = State::Lost; }
        void try_find_path() {
            this->state = State::Travelling;
            this->clear_path();
        }
        State current_state() const { return this->state; }

        u64 stored_count(Item::Type item) const {
            auto count = this->items.find(item);
            if(count == this->items.end()) { return 0; }
            return count->second;
        }
        u64 total_stored_count() const {
            u64 count = 0;
            for(const auto& stored: this->items) {
                count += stored.second;
            }
            return count;
        }
        void add_stored(Item::Type item, u64 amount) { this->items[item] += amount; }
        void remove_stored(Item::Type item, u64 amount) { this->items[item] -= amount; }
        void set_stored(Item::Type item, u64 amount) { this->items[item] = amount; }
        const std::unordered_map<Item::Type, u64>& stored_items() const {
            return this->items;
        }

        const Target* target() const {
            if(this->targets.size() == 0) { return nullptr; }
            return &this->targets[this->curr_target_i]; 
        }
        std::optional<u64> target_i() const {
            if(this->targets.size() == 0) { return std::nullopt; }
            return this->curr_target_i; 
        }
        void wrap_around_target_i() {
            if(this->targets.size() == 0) { return; }
            this->curr_target_i %= this->targets.size();
        }

        void update(
            engine::Scene& scene, const engine::Window& window,
            ComplexBank& complexes, const Terrain& terrain,
            bool is_visible
        );

        void render(
            Renderer& renderer, engine::Scene& scene, 
            const engine::Window& window,
            engine::Rendering rendering = engine::Rendering::Surfaces,
            const engine::Texture* override_texture = nullptr
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
            const Settings& settings,
            const Serialized& serialized, const engine::Arena& buffer,
            const Terrain& terrain
        );

        std::optional<std::vector<Vec<3>>> find_path_to(
            const Vec<3>& start,
            const Complex& target, const Terrain& terrain
        );

        void find_carriage_path(
            Carriage& carriage, 
            const ComplexBank& complexes, const Terrain& terrain, Toasts& toasts
        );
        void refind_all_paths(
            const ComplexBank& complexes, const Terrain& terrain, Toasts& toasts
        );
        
        void update_all(
            const Vec<3>& observer, f64 draw_distance,
            engine::Scene& scene, const engine::Window& window, 
            ComplexBank& complexes, const Terrain& terrain, Toasts& toasts
        );

        void render_all_around(
            const Vec<3>& observer, f64 draw_distance,
            Renderer& renderer, engine::Scene& scene, 
            const engine::Window& window
        );

        std::optional<size_t> find_selected_carriage(
            Vec<2> cursor_pos_ndc, const Renderer& renderer
        ) const;

        Serialized serialize(engine::Arena& buffer) const;

    };

}