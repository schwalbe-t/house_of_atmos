#include <engine/window.hpp>

// for testing
#include <glad/gl.h>
#include <engine/logging.hpp>
#include <GLFW/glfw3.h>

using namespace houseofatmos::engine;

static const f64 resolution = 100;

int main(int argc, char** argv) {
    auto window = Window(1280, 720, "House of Atmos", true);
    auto shader = Shader(
        "#version 110 \n"
        "attribute vec3 v_pos; \n"
        "attribute vec3 v_color; \n"
        "varying vec3 f_color; \n"
        "void main() { \n"
        "    gl_Position = vec4(v_pos, 1.0); \n"
        "    f_color = v_color; \n"
        "}",
        "#version 110 \n"
        "varying vec3 f_color; \n"
        "void main() { \n"
        "    gl_FragColor = vec4(f_color, 1.0); \n"
        "}"
    );
    auto mesh = Mesh { 3, 3 };
    mesh.add_element(
        mesh.add_vertex({ -0.5,  0.5, 0,   1, 0, 0 }),
        mesh.add_vertex({  0.5,  0.5, 0,   0, 1, 0 }),
        mesh.add_vertex({  0.0, -0.5, 0,   0, 0, 1 })
    );
    mesh.submit();
    auto target = Texture(100, 100);
    while(window.next_frame()) {
        target.resize_fast(ceil(resolution * window.width() / window.height()), resolution);
        target.clear_color(Vec<4>(0.5, 0.5, 0.5, 1.0));
        target.clear_depth(INFINITY);
        mesh.render(shader, target);
        window.show_texture(target);
    }
}