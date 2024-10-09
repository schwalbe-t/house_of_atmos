
#include "engine/engine.hpp"

using namespace houseofatmos;
using namespace houseofatmos::engine::math;

#include <iostream>
#include <cmath>
#include <chrono>

int main() {
    engine::init("House of Atmos", 800, 600, 60);
    auto main_buffer = engine::rendering::FrameBuffer(800, 600);
    auto sub_buffer = engine::rendering::FrameBuffer(200, 150);
    Vec<3> a = Vec<3>(-30.0, -30.0, 1.0);
    Vec<3> b = Vec<3>( 30.0, -30.0, 1.0);
    Vec<3> c = Vec<3>(-30.0,  30.0, 1.0);
    Vec<3> d = Vec<3>( 30.0,  30.0, 1.0);
    Mat<3> translation = Mat<3>(
        1.0, 0.0, 100.0,
        0.0, 1.0,  75.0,
        0.0, 0.0,   1.0
    );
    while(engine::is_running()) {
        double r = GetFrameTime();
        Mat<3> rotation = Mat<3>(
            cos(r), -sin(r), 0.0,
            sin(r),  cos(r), 0.0,
               0.0,     0.0, 1.0
        );
        auto before = std::chrono::system_clock::now();
        a = rotation * a;
        b = rotation * b;
        c = rotation * c;
        d = rotation * d;
        main_buffer.clear();
        sub_buffer.clear();
        sub_buffer.draw_triangle(
            (translation * a).swizzle<2>("xy"), 
            (translation * b).swizzle<2>("xy"), 
            (translation * c).swizzle<2>("xy"), 
            GREEN
        );
        sub_buffer.draw_triangle(
            (translation * b).swizzle<2>("xy"), 
            (translation * c).swizzle<2>("xy"), 
            (translation * d).swizzle<2>("xy"), 
            BLUE
        );
        sub_buffer.set_pixel_at(100, 75, WHITE);
        main_buffer.blit_buffer(sub_buffer, 0, 0, 800, 600);
        engine::display_buffer(&main_buffer);
        auto after = std::chrono::system_clock::now();
        std::cout << "Frame took " << std::chrono::duration_cast<std::chrono::nanoseconds>(after - before).count() << "ns" << std::endl;
    }
    engine::stop();
}























