
#pragma once

#include "nums.hpp"
#include <vector>
#include <span>
#include <cstring>

namespace houseofatmos::engine {
    
    struct Arena {
        
        private:
        std::vector<u8> buffer = std::vector<u8>(256);
        u64 next_offset = 0;

        void add_size(size_t n) {
            while(this->buffer.size() - this->next_offset < n) {
                this->buffer.resize(this->buffer.size() * 2);
            }
        }

        public:
        Arena() {}
        Arena(const std::vector<char>& data) {
            this->buffer.resize(data.size());
            std::memcpy(
                (void*) this->buffer.data(), (void*) data.data(), data.size()
            );
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

        template<typename T>
        u64 alloc(const T& value) {
            return this->alloc_array<T>(std::span<const T>(&value, 1));
        }

        template<typename T>
        std::span<const T> at(u64 offset, u64 count) const {
            const T* data = (const T*) (this->buffer.data() + offset);
            return std::span<const T>(data, count);
        }

        template<typename T>
        std::span<T> at(u64 offset, u64 count) {
            T* data = (T*) (this->buffer.data() + offset);
            return std::span<T>(data, count);
        }

        template<typename T>
        const T& at(u64 offset) const {
            return *(this->at<T>(offset, 1).data());
        }

        template<typename T>
        T& at(u64 offset) {
            return *(this->at<T>(offset, 1).data());
        }

    };

}