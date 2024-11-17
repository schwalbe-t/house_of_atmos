
#include "logging.hpp"
#include <iostream>

namespace houseofatmos::engine::logging {

    void raylib(int log_level, const char* text, va_list args) {
        if(log_level == 3) { info(std::string(text)); }
        else if(log_level == 4) { warning(std::string(text)); }
        else if(log_level >= 5) { error(std::string(text)); }
    }

    void info(std::string message) {
        std::cout << "    (info) " << message << std::endl;
    }

    void warning(std::string message) {
        std::cout << "(!) [WARNING] " << message << std::endl;
    }
    
    void error(std::string message) {
        std::cout << "(!) [ERROR] " << message << std::endl;
        std::abort();
    }

}