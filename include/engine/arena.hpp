
#pragma once

#include "nums.hpp"
#include <vector>
#include <unordered_map>
#include <span>
#include <cstring>

namespace houseofatmos::engine {
    
    struct Arena {
        
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
        Arena(const std::vector<char>& data) {
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
        u64 alloc_array(const T* data, size_t count) {
            u64 offset = this->next_offset;
            u64 n = count * sizeof(T);
            this->add_size(n);
            if(data != nullptr) {
                u8* dest = this->buffer.data() + this->next_offset;
                std::memcpy((void*) dest, (void*) data, n);
            }
            this->next_offset += n;
            return offset;
        }

        template<typename T>
        u64 alloc_array(std::span<const T> values) {
            return this->alloc_array<T>(values.data(), values.size());
        }

        template<class T, class Allocator>
        u64 alloc_array(const std::vector<T, Allocator>& values) {
            return this->alloc_array<T>(values.data(), values.size());
        }

        template<
            class K, class V, 
            class Hash, class KeyEqual, class Allocator
        > 
        u64 alloc_map(
            const std::unordered_map<K, V, Hash, KeyEqual, Allocator>& map
        ) {
            u64 offset = this->next_offset;
            for(std::pair<K, V> pair: map) {
                size_t n = sizeof(std::pair<K, V>);
                u8* dest = this->buffer.data() + this->next_offset;
                std::memcpy((void*) dest, (void*) &pair, n);
                this->next_offset += n;
            }
            return offset;
        }

        template<typename T>
        u64 alloc(const T& value) {
            return this->alloc_array<T>(std::span<const T>(&value, 1));
        }

        template<typename T>
        std::span<const T> array_at(u64 offset, u64 count) const {
            const T* data = (const T*) (this->buffer.data() + offset);
            return std::span<const T>(data, count);
        }

        template<typename T>
        std::span<T> array_at(u64 offset, u64 count) {
            T* data = (T*) (this->buffer.data() + offset);
            return std::span<T>(data, count);
        }

        template<class T, class Allocator>
        void copy_array_at_into(
            u64 offset, u64 count,
            std::vector<T, Allocator>& dest_array
        ) const {
            const T* data = (T*) (this->buffer.data() + offset);
            size_t dest_offset = dest_array.size();
            dest_array.resize(dest_offset + count);
            size_t n = sizeof(T) * count;
            std::memcpy(
                (void*) (dest_array.data() + dest_offset), (void*) data, n
            );
        }

        template<typename T>
        const T& value_at(u64 offset) const {
            return *(this->array_at<T>(offset, 1).data());
        }

        template<typename T>
        T& value_at(u64 offset) {
            return *(this->array_at<T>(offset, 1).data());
        }

        template<
            class K, class V, 
            class Hash, class KeyEqual, class Allocator
        > 
        void copy_map_at_into(
            u64 offset, u64 count,
            std::unordered_map<K, V, Hash, KeyEqual, Allocator>& dest_map
        ) const {
            std::span<const std::pair<K, V>> data = std::span(
                (const std::pair<K, V>*) (this->buffer.data() + offset), 
                count
            );
            dest_map.reserve(dest_map.size() + data.size());
            for(const std::pair<K, V>& pair: data) {
                dest_map[pair.first] = pair.second;
            }
        }

    };

}