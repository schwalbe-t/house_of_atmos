
#pragma once

#include <engine/arena.hpp>
#include <engine/window.hpp>
#include "complex_id.hpp"
#include "population.hpp"
#include "../balance.hpp"
#include "../research/research.hpp"
#include "terrain.hpp"
#include "item.hpp"
#include <vector>
#include <utility>
#include <unordered_map>
#include <optional>

namespace houseofatmos::world {

    using namespace houseofatmos;


    struct Conversion {
        struct Serialized {
            engine::Arena::Array<Item::Stack> inputs;
            engine::Arena::Array<Item::Stack> outputs;
            f64 period, passed;
        };

        Conversion(
            std::vector<Item::Stack> inputs, 
            std::vector<Item::Stack> outputs, 
            f64 period
        ) {
            this->inputs = std::move(inputs);
            this->outputs = std::move(outputs);
            this->period = period;
            this->passed = 0.0;
        }
        Conversion(const Serialized& serialized, const engine::Arena& buffer);

        std::vector<Item::Stack> inputs;
        std::vector<Item::Stack> outputs;
        f64 period, passed;

        Serialized serialize(engine::Arena& arena) const;
    };


    struct Complex {

        public:
        struct Member {
            struct Serialized {
                engine::Arena::Array<Conversion::Serialized> conversions;
            };

            Member(std::vector<Conversion> conversions) {
                this->conversions = conversions;
            }
            Member(const Serialized& serialized, const engine::Arena& buffer);

            std::vector<Conversion> conversions;

            Serialized serialize(engine::Arena& buffer) const;
        };

        static inline const f64 max_building_dist = 4.0;
        static inline const f64 max_diameter = 10.0;


        struct Serialized {
            engine::Arena::Array<
                std::pair<std::pair<u64, u64>, Member::Serialized>
            > members;
            engine::Arena::Map<Item::Type, u64> storage;
            bool free;
        };

        private:
        std::vector<std::pair<std::pair<u64, u64>, Member>> members;
        std::unordered_map<Item::Type, u64> storage;
        bool free;

        public:
        Complex();
        Complex(const Serialized& serialized, const engine::Arena& buffer);

        bool is_free() const { return this->free; }
        void set_free(bool is_free) { this->free = is_free; }

        std::pair<u64, u64> closest_member_to(
            u64 tile_x, u64 tile_z, f64* dist_out = nullptr
        ) const;
        std::pair<u64, u64> farthest_member_to(
            u64 tile_x, u64 tile_z, f64* dist_out = nullptr
        ) const;
        f64 distance_to(u64 tile_x, u64 tile_z) const;
        f64 farthest_distance_to(u64 tile_x, u64 tile_z) const;
        void add_member(u64 tile_x, u64 tile_z, Member member);
        void remove_member(u64 tile_x, u64 tile_z);
        bool has_member_at(u64 tile_x, u64 tile_z) const;
        Member& member_at(u64 tile_x, u64 tile_z);
        const Member& member_at(u64 tile_x, u64 tile_z) const;
        size_t member_count() const;
        std::span<const std::pair<std::pair<u64, u64>, Member>> get_members() const;
        u64 capacity(const Terrain& terrain) const;
        u64 free_capacity(Item::Type item, const Terrain& terrain) const;
        u64 stored_count(Item::Type item) const;
        u64 add_stored(Item::Type item, u64 amount, const Terrain& terrain);
        void remove_stored(Item::Type item, u64 amount);
        void set_stored(Item::Type item, u64 amount);
        const std::unordered_map<Item::Type, u64>& stored_items() const;

        std::unordered_map<Item::Type, f64> compute_throughput() const;

        void update(
            const engine::Window& window, Balance& balance, 
            research::Research& research, const Terrain& terrain,
            Toasts& toasts, PopulationManager& populations, ComplexId id
        );

        Serialized serialize(engine::Arena& buffer) const;

    };


    struct ComplexBank {

        struct Serialized {
            engine::Arena::Array<Complex::Serialized> complexes;
            engine::Arena::Array<ComplexId> free_indices;
        };

        private:
        std::vector<Complex> complexes;
        std::vector<ComplexId> free_indices;

        public:
        ComplexBank();
        ComplexBank(const Serialized& serialized, const engine::Arena& buffer);

        ComplexId create_complex();
        std::optional<ComplexId> closest_to(u64 tile_x, u64 tile_z) const;
        Complex& get(ComplexId complex);
        const Complex& get(ComplexId complex) const;
        const Complex* get_arbitrary(u64 complex_i) const;
        void delete_complex(ComplexId complex);

        void update(
            const engine::Window& window, Balance& balance, 
            research::Research& research, const Terrain& terrain,
            Toasts& toasts, PopulationManager& populations
        );

        Serialized serialize(engine::Arena& buffer) const;

    };

}