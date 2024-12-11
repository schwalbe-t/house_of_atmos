
#pragma once

#include "nums.hpp"

namespace houseofatmos::engine {

    struct Shader {

        private:
        u64 vert_id;
        u64 frag_id;
        u64 prog_id;

        public:
        Shader(const char* vertex, const char* fragment);
        Shader(const Shader& other) = delete;
        Shader& operator=(const Shader& other) = delete;
        ~Shader();

    };

}