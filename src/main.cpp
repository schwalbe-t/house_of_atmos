
#include "engine/engine.hpp"

using namespace houseofatmos;
using namespace houseofatmos::engine::math;

#include <iostream>
#include <chrono>

int main() {
    engine::init("House of Atmos", 800, 600, 60);
    auto buffer = engine::rendering::FrameBuffer(800, 600);
    while(engine::is_running()) {
        buffer.clear();
        auto before = std::chrono::system_clock::now();
        buffer.draw_triangle(
            Vec<2>(50.0, 350.0), 
            Vec<2>(350.0, 50.0), 
            Vec<2>(GetMousePosition().x, GetMousePosition().y), 
            BLUE
        );
        auto after = std::chrono::system_clock::now();
        std::cout << "Took " << std::chrono::duration_cast<std::chrono::nanoseconds>(after - before).count() << "ns to render triangle" << std::endl;
        engine::display_buffer(&buffer);
    }
    engine::stop();
}























