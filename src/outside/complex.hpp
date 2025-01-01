
#pragma once

#include <engine/arena.hpp>
#include <engine/nums.hpp>
#include <vector>
#include <utility>
#include <unordered_map>
#include <optional>

namespace houseofatmos::outside {

    enum struct Item {
        Grain, Flour, Bread,
        Ore, Metal, Tools
    };


    struct Conversion {
        struct Serialized {
            u64 inputs_count, inputs_offset;
            u64 outputs_count, outputs_offset;
            f64 time;
        };

        Conversion(const Serialized& serialized, const engine::Arena& buffer);

        std::vector<std::pair<u8, Item>> inputs;
        std::vector<std::pair<u8, Item>> outputs;
        f64 time;

        Serialized serialize(engine::Arena& arena) const;
    };


    struct Complex {

        private:
        struct hash_pair {
            template<class T1, class T2>
            size_t operator()(const std::pair<T1, T2>& p) const {
                // Hash the first element
                size_t hash1 = std::hash<T1>{}(p.first);
                // Hash the second element
                size_t hash2 = std::hash<T2>{}(p.second);
                // Combine the two hash values
                return hash1
                    ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
            }
        };


        public:
        struct Member {
            struct Serialized {
                u64 conversions_count, conversions_offset;
            };

            Member();
            Member(const Serialized& serialized, const engine::Arena& buffer);

            std::vector<Conversion> conversions;

            Serialized serialize(engine::Arena& buffer) const;
        };


        struct Serialized {
            u64 center_x, center_z;
            u64 members_count, members_offset;
            u64 storage_count, storage_offset;
        };

        Complex();
        Complex(const Serialized& serialized, const engine::Arena& buffer);
        
        u64 center_x, center_z;
        std::unordered_map<std::pair<u64, u64>, Member, hash_pair> members;
        std::unordered_map<Item, u64> storage;

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


        Serialized serialize(engine::Arena& buffer) const;

    };

}