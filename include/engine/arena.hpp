
#pragma once

#include "nums.hpp"
#include <vector>
#include <list>
#include <unordered_map>
#include <span>
#include <cstring>
#include <functional>

namespace houseofatmos::engine {
    
    struct Arena {
        
        template<typename T>
        struct Position { 
            u64 byte_offset; 
            
            Position(u64 byte_offset): byte_offset(byte_offset) {}
        };

        template<typename T>
        struct Array { 
            Position<T> position;
            u64 size;

            Array(Position<T> position, u64 size): 
                position(position), size(size) {}
        };

        template<typename K, typename V>
        struct Map { 
            Position<std::pair<K, V>> position;
            u64 size;

            Map(Position<std::pair<K, V>> position, u64 size): 
                position(position), size(size) {}
        };

        private:
        std::vector<u8> buffer;
        u64 next_offset;

        void add_size(size_t n) {
            while(this->buffer.size() - this->next_offset < n) {
                this->buffer.resize(this->buffer.size() * 2);
            }
        }

        public:
        Arena() {
            this->buffer = std::vector<u8>(256);
            this->next_offset = 0;
        }
        Arena(std::span<const char> data) {
            this->buffer.resize(data.size());
            size_t n = data.size() * sizeof(char);
            std::memcpy(
                (void*) this->buffer.data(), (void*) data.data(), n
            );
            this->next_offset = n;
        }

        std::span<const u8> data() const {
            return std::span(this->buffer.data(), this->next_offset); 
        }

        template<typename T>
        Array<T> alloc(const T* data, size_t count) {
            u64 offset = this->next_offset;
            u64 n = count * sizeof(T);
            this->add_size(n);
            if(data != nullptr) {
                u8* dest = this->buffer.data() + this->next_offset;
                std::memcpy((void*) dest, (void*) data, n);
            }
            this->next_offset += n;
            return Array<T>(Position<T>(offset), count);
        }

        template<typename T>
        Array<T> alloc(std::span<const T> values) {
            return this->alloc<T>(values.data(), values.size());
        }

        template<class T, class Allocator>
        Array<T> alloc(const std::vector<T, Allocator>& values) {
            return this->alloc<T>(values.data(), values.size());
        }
        
        template<typename T, typename S, typename C>
        Array<S> alloc(
            const C& values,
            const std::function<S (const T&)>& conv
                = [](const auto& v) { return v; }
        ) {
            u64 start = this->next_offset;
            size_t n = values.size() * sizeof(S);
            this->add_size(n);
            this->next_offset += n;
            Position<S> current = Position<S>(start);
            for(const T& value: values) {
                S v = conv(value);
                // 'conv' can re-allocate the buffer!
                // only actually get the location reference after 'conv' called
                this->get(current) = v;
                current.byte_offset += sizeof(S);
            }
            return Array<S>(Position<S>(start), values.size());
        }

        template<
            class K, class V, 
            class Hash, class KeyEqual, class Allocator
        > 
        Map<K, V> alloc(
            const std::unordered_map<K, V, Hash, KeyEqual, Allocator>& map
        ) {
            u64 offset = this->next_offset;
            for(std::pair<K, V> pair: map) {
                size_t n = sizeof(std::pair<K, V>);
                u8* dest = this->buffer.data() + this->next_offset;
                std::memcpy((void*) dest, (void*) &pair, n);
                this->next_offset += n;
            }
            return Map<K, V>(Position<std::pair<K, V>>(offset), map.size());
        }

        template<typename T>
        Position<T> alloc() { 
            return this->alloc<T>(nullptr, 1).position; 
        }

        template<typename T>
        Position<T> alloc(const T& value) {
            return this->alloc<T>(&value, 1).position;
        }

        template<typename T>
        std::span<const T> get(const Array<T>& array) const {
            const T* data 
                = (const T*) (this->buffer.data() + array.position.byte_offset);
            return std::span<const T>(data, array.size);
        }

        template<typename T>
        std::span<T> get(const Array<T>& array) {
            T* data = (T*) (this->buffer.data() + array.position.byte_offset);
            return std::span<T>(data, array.size);
        }

        template<class T, class Allocator>
        void copy_into(
            const Array<T>& array,
            std::vector<T, Allocator>& dest
        ) const {
            size_t dest_offset = dest.size();
            dest.resize(dest_offset + array.size);
            std::memcpy(
                (void*) (dest.data() + dest_offset),
                (void*) this->get(array).data(), 
                sizeof(T) * array.size
            );
        }

        template<typename S, typename R, typename D>
        void copy_into(
            const Array<S>& array,
            D& dest,
            const std::function<R (const S&)>& conv 
                = [](const auto& v) { return v; }
        ) const {
            for(const S& value: this->get(array)) {
                dest.push_back(conv(value));
            }
        }

        template<typename T>
        const T& get(Position<T> position) const {
            return this->get<T>(Array<T>(position, 1))[0];
        }

        template<typename T>
        T& get(Position<T> position) {
            return this->get<T>(Array<T>(position, 1))[0];
        }

        template<
            class K, class V, 
            class Hash, class KeyEqual, class Allocator
        > 
        void copy_into(
            const Map<K, V>& map,
            std::unordered_map<K, V, Hash, KeyEqual, Allocator>& dest
        ) const {
            std::span<const std::pair<K, V>> data 
                = this->get(Array(map.position, map.size));
            dest.reserve(dest.size() + data.size());
            for(const auto& [key, value]: data) {
                dest[key] = value;
            }
        }

    };

}