
#pragma once

#include "nums.hpp"

namespace houseofatmos::engine {

    struct Window {

        private:
        void* ptr;


        public:
        Window(u32 width, u32 height, const char* name, bool vsync);
        Window(const Window& other) = delete;
        Window& operator=(const Window& other) = delete;
        ~Window();

        bool is_open();
        
        void* internal_ptr();

    };

}