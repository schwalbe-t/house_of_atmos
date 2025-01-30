
#include <engine/window.hpp>
#include "outside/outside.hpp"
#include <filesystem>

using namespace houseofatmos;

int main() {
    
    auto window = engine::Window(1280, 720, "House of Atmos");
    if(std::filesystem::exists(outside::Outside::save_location)) {
        std::vector<char> bytes = engine::GenericResource::read_bytes(outside::Outside::save_location);
        auto buffer = engine::Arena(bytes);
        window.set_scene(std::make_unique<outside::Outside>(buffer));
    } else {
        window.set_scene(std::make_unique<outside::Outside>());
    }
    window.start();
}