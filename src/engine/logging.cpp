
#include <engine/logging.hpp>
#include <iostream>

namespace houseofatmos::engine {

    void debug(std::string message) {
        std::cout << "    {debug} " << message << std::endl;
    }

    void info(std::string message) {
        std::cout << "    (info) " << message << std::endl;
    }

    void warning(std::string message) {
        std::cout << " !  <WARNING> " << message << std::endl;
    }

    [[noreturn]]
    void error(std::string message) {
        std::cout << "!!! [ERROR] " << message << std::endl;
        std::abort();
    }

    namespace internal {
        
        void glfw_error(int error, const char* description) {
            (void) error;
            std::cout << "!!! [ERROR] (GLFW) " 
                << std::string(description)
                << std::endl;
        }

    }

}