
#include "engine/engine.hpp"

using namespace houseofatmos;

int main() {
    engine::init("House of Atmos", 800, 600, 60);
    auto buffer = engine::rendering::FrameBuffer(800, 600);
    for(int n = 0; n < 100; n += 1) {
        buffer.set_pixel_at(n, n, WHITE);
    }
    while(engine::is_running()) {
        engine::display_buffer(&buffer);
    }
    engine::stop();
}























