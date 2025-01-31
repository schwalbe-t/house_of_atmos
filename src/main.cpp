
#include <engine/window.hpp>
#include "outside/outside.hpp"
#include "pause_menu.hpp"
#include <filesystem>

using namespace houseofatmos;

int main() {
    auto window = engine::Window(1280, 720, "House of Atmos");
    const engine::Localization::LoadArgs local = {
        "res/localization.json", "en"
    };
    window.set_scene(std::make_unique<outside::Outside>(local));
    window.start();
}