
#include <engine/math.hpp>
#include "complex.hpp"

using namespace houseofatmos::engine::math;

namespace houseofatmos::world {

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
        this->period = serialized.period;
        this->passed = serialized.passed;
    }

    Conversion::Serialized Conversion::serialize(engine::Arena& buffer) const {
        return (Serialized) {
            this->inputs.size(), buffer.alloc_array(this->inputs),
            this->outputs.size(), buffer.alloc_array(this->outputs),
            this->period, this->passed
        };
    }



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
        this->free = false;
    }

    Complex::Complex(const Serialized& serialized, const engine::Arena& buffer) {
        std::vector<std::pair<std::pair<u64, u64>, Member::Serialized>> members;
        buffer.copy_array_at_into(
            serialized.members_offset, serialized.members_count, members
        );
        for(const auto& [location, member]: members) {
            this->members.push_back({ location, Member(member, buffer) });
        }
        buffer.copy_map_at_into(
            serialized.storage_offset, serialized.storage_count, this->storage
        );
        this->free = serialized.free;
    }

    Complex::Serialized Complex::serialize(engine::Arena& buffer) const {
        std::vector<std::pair<std::pair<u64, u64>, Member::Serialized>> members;
        members.reserve(this->members.size());
        for(const auto& [location, member]: this->members) {
            members.push_back({ location, member.serialize(buffer) });
        }
        return (Serialized) {
            this->members.size(), buffer.alloc_array(members),
            this->storage.size(), buffer.alloc_map(this->storage),
            this->free
        };
    }

    std::pair<u64, u64> Complex::closest_member_to(
        u64 tile_x, u64 tile_z, f64* dist_out
    ) const {
        assert(this->members.size() > 0);
        f64 min_distance = INFINITY;
        std::pair<u64, u64> closest;
        for(const auto& member: this->members) {
            const auto& [member_x, member_z] = member.first;
            Vec<2> difference = Vec<2>(member_x, member_z) 
                - Vec<2>(tile_x, tile_z);
            f64 distance = difference.len();
            if(distance >= min_distance) { continue; }
            min_distance = distance;
            closest = member.first;
        }
        if(dist_out != nullptr) {
            *dist_out = min_distance;
        }
        return closest;
    }

    std::pair<u64, u64> Complex::farthest_member_to(
        u64 tile_x, u64 tile_z, f64* dist_out
    ) const {
        assert(this->members.size() > 0);
        f64 max_distance = 0.0;
        std::pair<u64, u64> farthest;
        for(const auto& member: this->members) {
            const auto& [member_x, member_z] = member.first;
            Vec<2> difference = Vec<2>(member_x, member_z) 
                - Vec<2>(tile_x, tile_z);
            f64 distance = difference.len();
            if(distance <= max_distance) { continue; }
            max_distance = distance;
            farthest = member.first;
        }
        if(dist_out != nullptr) {
            *dist_out = max_distance;
        }
        return farthest;
    }

    f64 Complex::distance_to(u64 tile_x, u64 tile_z) const {
        f64 distance;
        (void) this->closest_member_to(tile_x, tile_z, &distance);
        return distance;
    }

    f64 Complex::farthest_distance_to(u64 tile_x, u64 tile_z) const {
        f64 distance;
        (void) this->farthest_member_to(tile_x, tile_z, &distance);
        return distance;
    }

    void Complex::add_member(u64 tile_x, u64 tile_z, Member member) {
        for(auto& member: this->members) {
            const auto& [member_x, member_z] = member.first;
            if(member_x != tile_x || member_z != tile_z) { continue; }
            engine::error("Complex already contains a member at tile ["
                + std::to_string(tile_x) + ", " + std::to_string(tile_z) + "]"
            );
        }
        this->members.push_back({{ tile_x, tile_z }, member });
    }

    void Complex::remove_member(u64 tile_x, u64 tile_z) {
        for(size_t mem_i = 0; mem_i < this->members.size(); mem_i += 1) {
            const auto& [member_x, member_z] = this->members.at(mem_i).first;
            if(member_x != tile_x || member_z != tile_z) { continue; }
            this->members.erase(this->members.begin() + mem_i);
            return;
        }
        engine::error("Complex does not contain a member at tile ["
            + std::to_string(tile_x) + ", " + std::to_string(tile_z) + "]"
        );
    }

    bool Complex::has_member_at(u64 tile_x, u64 tile_z) const {
        for(auto& member: this->members) {
            const auto& [member_x, member_z] = member.first;
            if(member_x != tile_x || member_z != tile_z) { continue; }
            return true;
        }
        return false;
    }

    Complex::Member& Complex::member_at(u64 tile_x, u64 tile_z) {
        for(auto& member: this->members) {
            const auto& [member_x, member_z] = member.first;
            if(member_x != tile_x || member_z != tile_z) { continue; }
            return member.second;
        }
        engine::error("Complex does not contain a member at tile ["
            + std::to_string(tile_x) + ", " + std::to_string(tile_z) + "]"
        );
    }

    const Complex::Member& Complex::member_at(u64 tile_x, u64 tile_z) const {
        return (const Member&) ((Complex*) this)->member_at(tile_x, tile_z);
    }

    size_t Complex::member_count() const { return this->members.size(); }

    std::span<const std::pair<std::pair<u64, u64>, Complex::Member>>
        Complex::get_members() const { return this->members; }

    u64 Complex::capacity(const Terrain& terrain) const {
        u64 sum = 0;
        for(const auto& member: this->members) {
            auto [x, z] = member.first;
            const Building* building = terrain.building_at((i64) x, (i64) z);
            if(building == nullptr) { continue; }
            sum += Building::types().at((size_t) building->type).capacity;
        }
        return sum;
    }

    u64 Complex::free_capacity(Item::Type item, const Terrain& terrain) const {
        u64 capacity = this->capacity(terrain);
        u64 stored = this->stored_count(item);
        if(stored >= capacity) { return 0; }
        return capacity - stored;
    }

    u64 Complex::stored_count(Item::Type item) const {
        auto count = this->storage.find(item);
        if(count == this->storage.end()) { return 0; }
        return count->second;
    }

    u64 Complex::add_stored(
        Item::Type item, u64 amount, const Terrain& terrain
    ) {
        u64 added = std::min(this->free_capacity(item, terrain), amount);
        this->storage[item] += added;
        return added;
    }

    void Complex::remove_stored(Item::Type item, u64 amount) {
        this->storage[item] -= amount;
    }

    void Complex::set_stored(Item::Type item, u64 amount) {
        this->storage[item] = amount;
    }

    const std::unordered_map<Item::Type, u64>& Complex::stored_items() const {
        return this->storage;
    }

    std::unordered_map<Item::Type, f64> Complex::compute_throughput() const {
        auto result = std::unordered_map<Item::Type, f64>();
        for(const auto& member: this->members) {
            for(const Conversion& conversion: member.second.conversions) {
                for(const auto& [count, item]: conversion.inputs) {
                    result[item] -= (f64) count / conversion.period;
                }
                for(const auto& [count, item]: conversion.outputs) {
                    result[item] += (f64) count / conversion.period;
                }
            }
        }
        return result;
    }

    void Complex::update(
        const engine::Window& window, Balance& balance, 
        research::Research& research, const Terrain& terrain, Toasts& toasts
    ) {
        for(auto& member: this->members) {
            for(Conversion& conversion: member.second.conversions) {
                conversion.passed += window.delta_time();
                u64 passed_times = (u64) (conversion.passed / conversion.period);
                if(passed_times == 0) { continue; }
                conversion.passed = fmod(conversion.passed, conversion.period);
                u64 allowed_times = passed_times;
                for(auto& [count, item]: conversion.inputs) {
                    u64 present_count = this->stored_count(item) / count;
                    allowed_times = std::min(allowed_times, present_count);
                }
                for(auto& [count, item]: conversion.outputs) {
                    u64 capacity_count 
                        = this->free_capacity(item, terrain) / count;
                    allowed_times = std::min(allowed_times, capacity_count);
                }
                if(allowed_times == 0) { continue; }
                for(auto& [count, item]: conversion.inputs) {
                    u64 consumed = allowed_times * count;
                    this->remove_stored(item, consumed);
                }
                for(auto& [count, item]: conversion.outputs) {
                    bool storable = Item::types().at(item).storable;
                    u64 produced = allowed_times * count;
                    research.report_item_production(item, produced, toasts);
                    if(item == Item::Coins) {
                        balance.add_coins_silent(produced);
                    }
                    if(!storable) { continue; }
                    this->add_stored(item, produced, terrain);
                }
            }
        }
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
            ComplexId id = this->free_indices.at(this->free_indices.size() - 1);
            this->get(id).set_free(false);
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
            if(complex.is_free()) { continue; }
            f64 distance = complex.distance_to(tile_x, tile_z);
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

    const Complex* ComplexBank::get_arbitrary(u64 complex_i) const {
        if(complex_i >= this->complexes.size()) { return nullptr; }
        return &this->complexes[complex_i];   
    }

    void ComplexBank::update(
        const engine::Window& window, Balance& balance, 
        research::Research& research, const Terrain& terrain,
        Toasts& toasts
    ) {
        for(Complex& complex: this->complexes) {
            if(complex.is_free()) { continue; }
            complex.update(window, balance, research, terrain, toasts);
        }
    }

    void ComplexBank::delete_complex(ComplexId complex) {
        Complex& deleted = this->complexes.at(complex.index);
        deleted = Complex();
        deleted.set_free(true);
        this->free_indices.push_back(complex);
    }

}