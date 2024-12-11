
#include "engine/window.hpp"
#include "engine/rendering.hpp"

using namespace houseofatmos;

void display() {

}

int main(int argc, char** argv) {
    auto window = engine::Window(1280, 720, "House of Atmos", true);
    auto shader = engine::Shader(
        "#version 430 \n"
        "in vec3 pos; \n"
        "void main() { \n"
        "    gl_Position = vec4(pos, 1); \n"
        "}",
        "#version 430 \n"
        "out vec4 color; \n"
        "void main() { \n"
        "    color = vec4(1, 0, 0, 1); \n"
        "}"
    );
    while(window.is_open()) {
        
    }
}