
#include <engine/window.hpp>
#include "outside/outside.hpp"

using namespace houseofatmos;

int main() {
    auto window = engine::Window(1280, 720, "House of Atmos");
    window.set_scene(std::make_unique<outside::Outside>(Settings()));
    window.start();
}