
#pragma once

#include <engine/arena.hpp>
#include <engine/window.hpp>
#include <vector>
#include <utility>
#include <unordered_map>
#include <optional>

namespace houseofatmos::outside {

    using namespace houseofatmos;


    enum struct Item {
        Barley, /* -> */ Malt, /* -> */ Beer,
        Wheat, /* -> */ Flour, /* -> */ Bread,
        Hematite, /* and */ Coal, /* -> */ Steel, /* -> */ Armor,
                                                  /* or */ Tools
    };


    struct Conversion {
        struct Serialized {
            u64 inputs_count, inputs_offset;
            u64 outputs_count, outputs_offset;
            f64 period, passed;
        };

        Conversion(
            std::vector<std::pair<u8, Item>> inputs, 
            std::vector<std::pair<u8, Item>> outputs, 
            f64 period
        ) {
            this->inputs = std::move(inputs);
            this->outputs = std::move(outputs);
            this->period = period;
            this->passed = 0.0;
        }
        Conversion(const Serialized& serialized, const engine::Arena& buffer);

        std::vector<std::pair<u8, Item>> inputs;
        std::vector<std::pair<u8, Item>> outputs;
        f64 period, passed;

        Serialized serialize(engine::Arena& arena) const;
    };


    struct Complex {

        public:
        struct Member {
            struct Serialized {
                u64 conversions_count, conversions_offset;
            };

            Member(std::vector<Conversion> conversions) {
                this->conversions = conversions;
            }
            Member(const Serialized& serialized, const engine::Arena& buffer);

            std::vector<Conversion> conversions;

            Serialized serialize(engine::Arena& buffer) const;
        };

        static inline const f64 max_building_dist = 4.0;


        struct Serialized {
            u64 members_count, members_offset;
            u64 storage_count, storage_offset;
        };

        private:
        std::vector<std::pair<std::pair<u64, u64>, Member>> members;
        std::unordered_map<Item, u64> storage;

        public:
        Complex();
        Complex(const Serialized& serialized, const engine::Arena& buffer);

        f64 distance_to(u64 tile_x, u64 tile_z) const;
        void add_member(u64 tile_x, u64 tile_z, Member member);
        void remove_member(u64 tile_x, u64 tile_z);
        Member& member_at(u64 tile_x, u64 tile_z);
        const Member& member_at(u64 tile_x, u64 tile_z) const;
        size_t member_count() const;
        u64 stored_count(Item item) const;
        void add_stored(Item item, u64 amount);
        void remove_stored(Item item, u64 amount);
        void set_stored(Item item, u64 amount);

        void update(const engine::Window& window);

        Serialized serialize(engine::Arena& buffer) const;

    };


    struct ComplexId {
        u32 index;
    };

    struct ComplexBank {

        struct Serialized {
            u64 complexes_count, complexes_offset;
            u64 free_indices_count, free_indices_offset;
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
        void delete_complex(ComplexId complex);

        void update(const engine::Window& window);

        Serialized serialize(engine::Arena& buffer) const;

    };

}