
#include <engine/math.hpp>
#include "complex.hpp"

using namespace houseofatmos::engine::math;

namespace houseofatmos::outside {

    Conversion::Conversion(
        const Serialized& serialized, const engine::Arena& buffer
    ) {
        buffer.copy_array_at_into(
            serialized.inputs_offset, serialized.inputs_count,
            this->inputs
        );
        buffer.copy_array_at_into(
            serialized.outputs_offset, serialized.outputs_count,
            this->outputs
        );
        this->time = serialized.time;
    }

    Conversion::Serialized Conversion::serialize(engine::Arena& buffer) const {
        return (Serialized) {
            this->inputs.size(), buffer.alloc_array(this->inputs),
            this->outputs.size(), buffer.alloc_array(this->outputs),
            this->time
        };
    }



    Complex::Member::Member() {}

    Complex::Member::Member(
        const Serialized& serialized, const engine::Arena& buffer
    ) {
        std::span<const Conversion::Serialized> conversions = buffer
            .array_at<Conversion::Serialized>(
                serialized.conversions_offset, serialized.conversions_count
            );
        this->conversions.reserve(conversions.size());
        for(const Conversion::Serialized& conversion: conversions) {
            this->conversions.push_back(Conversion(conversion, buffer));
        }
    }

    Complex::Member::Serialized Complex::Member::serialize(
        engine::Arena& buffer
    ) const {
        std::vector<Conversion::Serialized> conversions;
        conversions.reserve(this->conversions.size());
        for(const Conversion& conversion: this->conversions) {
            conversions.push_back(conversion.serialize(buffer));
        }
        return (Serialized) {
            this->conversions.size(), buffer.alloc_array(conversions)
        };
    }



    Complex::Complex() {
        this->center_x = UINT32_MAX;
        this->center_z = UINT32_MAX;
    }

    Complex::Complex(const Serialized& serialized, const engine::Arena& buffer) {
        this->center_x = serialized.center_x;
        this->center_z = serialized.center_z;
        std::vector<std::pair<std::pair<u64, u64>, Member::Serialized>> members;
        buffer.copy_array_at_into(
            serialized.members_offset, serialized.members_count, members
        );
        for(const auto& [location, member]: members) {
            this->members[location] = Member(member, buffer);
        }
        buffer.copy_map_at_into(
            serialized.storage_offset, serialized.storage_count, this->storage
        );
    }

    Complex::Serialized Complex::serialize(engine::Arena& buffer) const {
        std::vector<std::pair<std::pair<u64, u64>, Member::Serialized>> members;
        members.reserve(this->members.size());
        for(const auto& [location, member]: this->members) {
            members.push_back({ location, member.serialize(buffer) });
        }
        return (Serialized) {
            this->center_x, this->center_z,
            this->members.size(), buffer.alloc_array(members),
            this->storage.size(), buffer.alloc_map(this->storage)
        };
    }



    ComplexBank::ComplexBank() {}

    ComplexBank::ComplexBank(const Serialized& serialized, const engine::Arena& buffer) {
        auto complexes = buffer.array_at<Complex::Serialized>(
            serialized.complexes_offset, serialized.complexes_count
        );
        this->complexes.reserve(complexes.size());
        for(const auto& complex: complexes) {
            this->complexes.push_back(Complex(complex, buffer));
        }
        buffer.copy_array_at_into(
            serialized.free_indices_offset, serialized.free_indices_count,
            this->free_indices
        );
    }

    ComplexBank::Serialized ComplexBank::serialize(engine::Arena& buffer) const {
        std::vector<Complex::Serialized> complexes;
        complexes.reserve(this->complexes.size());
        for(const auto& complex: this->complexes) {
            complexes.push_back(complex.serialize(buffer));
        }
        return (Serialized) {
            this->complexes.size(), buffer.alloc_array(complexes),
            this->free_indices.size(), buffer.alloc_array(this->free_indices)
        };
    }

    ComplexId ComplexBank::create_complex() {
        if(this->free_indices.size() > 0) {
            ComplexId id = this->free_indices.at(this->free_indices.size());
            this->free_indices.pop_back();
            return id;
        }
        ComplexId id = (ComplexId) { (u32) this->complexes.size() };
        this->complexes.push_back(Complex());
        return id;
    }

    std::optional<ComplexId> ComplexBank::closest_to(
        u64 tile_x, u64 tile_z
    ) const {
        std::optional<ComplexId> closest = std::nullopt;
        f64 closest_distance = INFINITY;
        for(size_t index = 0; index < this->complexes.size(); index += 1) {
            const Complex& complex = this->complexes.at(index);
            Vec<2> delta = Vec<2>(complex.center_x, complex.center_z)
                - Vec<2>(tile_x, tile_z);
            f64 distance = delta.len();
            if(closest_distance <= distance) { continue; }
            closest = (ComplexId) { (u32) index };
            closest_distance = distance;
        }
        return closest;
    }

    Complex& ComplexBank::get(ComplexId complex) {
        return this->complexes.at(complex.index);   
    }

    const Complex& ComplexBank::get(ComplexId complex) const {
        return this->complexes.at(complex.index);   
    }

    void ComplexBank::delete_complex(ComplexId complex) {
        this->complexes.at(complex.index) = Complex();
        this->free_indices.push_back(complex);
    }

}