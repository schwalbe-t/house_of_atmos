
#include <engine/window.hpp>
#include "outside/outside.hpp"
#include <filesystem>

using namespace houseofatmos;

int main() {
    engine::info("As there is no UI system yet, information will be visible here.");
    engine::info("It is recommended to keep this output visible while playing.");
    auto window = engine::Window(1280, 720, "House of Atmos");
    if(std::filesystem::exists(outside::Outside::save_location)) {
        std::vector<char> bytes = engine::GenericResource::read_bytes(outside::Outside::save_location);
        auto buffer = engine::Arena(bytes);
        window.set_scene(std::make_unique<outside::Outside>(buffer));
        engine::info(
            "Loaded save file at '" 
            + std::string(outside::Outside::save_location) 
            + "'. Deleting the file will reset your progress."
        );
    } else {
        window.set_scene(std::make_unique<outside::Outside>());
    }
    window.start();
}