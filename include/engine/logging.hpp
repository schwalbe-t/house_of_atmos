
#pragma once

#include <string>

namespace houseofatmos::engine {

    void debug(std::string message);

    void info(std::string message);

    void warning(std::string message);

    [[noreturn]]
    void error(std::string message);

    namespace internal {

        void glfw_error(int error, const char* description);

    }

}