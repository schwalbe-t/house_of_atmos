
#include <engine/window.hpp>
#include "main_menu/main_menu.hpp"

using namespace houseofatmos;

int main() {
    Settings settings = Settings::read_from_path(Settings::default_path);
    auto window = engine::Window(1280, 720, "House of Atmos");
    if(settings.fullscreen) { window.set_fullscreen(); }
    window.set_scene(std::make_unique<MainMenu>(std::move(settings)));
    window.start();
}