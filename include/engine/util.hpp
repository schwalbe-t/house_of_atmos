
#pragma once

#include "logging.hpp"

// internal utilities used by the engine
namespace houseofatmos::engine::util {

    template<typename T>
    using HandleDestructor = void (*)(T&);

    // implements moving and destruction of a resource
    template<typename T, HandleDestructor<T> Destructor>
    struct Handle {

        private:
        T value;
        bool empty;

        public:
        Handle(): empty(true) {}
        Handle(T value): value(value), empty(false) {}

        Handle(Handle&& other) noexcept {
            this->value = other.value;
            this->empty = other.empty;
            other.empty = true;
        }
        Handle& operator=(Handle&& other) noexcept {
            if(this == &other) { return *this; }
            if(!this->empty) {
                Destructor(this->value);
            }
            this->value = other.value;
            this->empty = other.empty;
            other.empty = true;
            return *this;
        }

        const T& operator*() const {
            if(this->empty) {
                error("Usage of an empty or moved handle!");
            }
            return this->value;
        }

        const T* operator->() const {
            if(this->empty) {
                error("Usage of an empty or moved handle!");
            }
            return &this->value;
        }

        bool is_empty() const { return this->empty; }

        ~Handle() {
            if(this->empty) { return; }
            this->empty = true;
            Destructor(this->value);
        }

    };

}