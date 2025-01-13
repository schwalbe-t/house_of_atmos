
#pragma once

#include <engine/nums.hpp>
#include <engine/logging.hpp>

namespace houseofatmos::outside {
    
    struct Zoom {
        enum Level {
            Near, Far
        };

        static f64 offset_of(Zoom::Level level) {
            switch(level) {
                case Zoom::Near: return 12;
                case Zoom::Far: return 40;
            }
            engine::error("Unhandled 'Zoom::Level' in 'Zoom::offset_of'");
        }
    };

}