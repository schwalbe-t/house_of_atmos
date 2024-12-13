
#include <engine/window.hpp>

using namespace houseofatmos::engine;

static const f64 resolution = 100;

int main(int argc, char** argv) {
    auto window = Window(1280, 720, "House of Atmos");
    auto shader = Shader(
        "#version 130 \n"
        "in vec3 v_pos; \n"
        "in vec3 v_color; \n"
        "out vec3 f_color; \n"
        "void main() { \n"
        "    gl_Position = vec4(v_pos, 1.0); \n"
        "    f_color = v_color; \n"
        "}",
        "#version 130 \n"
        "in vec3 f_color; \n"
        "uniform vec4 u_colors[3]; \n"
        "void main() { \n"
        "    gl_FragColor = u_colors[0] * round(f_color.x * 2) / 2 + u_colors[1] * round(f_color.y * 2) / 2 + u_colors[2] * round(f_color.z * 2) / 2; \n"
        "}"
    );
    auto colors = std::array {
        Vec<4>(1.0, 0.0, 0.0, 1.0),
        Vec<4>(0.0, 1.0, 0.0, 1.0),
        Vec<4>(0.0, 0.0, 1.0, 1.0)
    };
    shader.set_uniform("u_colors", colors);
    auto mesh = Mesh { 3, 3 };
    mesh.add_element(
        mesh.add_vertex({ -0.5,  0.5, 0,   1, 0, 0 }),
        mesh.add_vertex({  0.5,  0.5, 0,   0, 1, 0 }),
        mesh.add_vertex({  0.0, -0.5, 0,   0, 0, 1 })
    );
    mesh.submit();
    auto target = Texture(100, 100);
    while(window.next_frame()) {
        if(resolution < window.height()) {
            target.resize_fast(ceil(resolution * window.width() / window.height()), resolution);
        } else {
            target.resize_fast(window.width(), window.height());
        }
        target.clear_color(Vec<4>(1.0, 1.0, 1.0, 1.0));
        target.clear_depth(INFINITY);
        mesh.render(shader, target);
        window.show_texture(target);
    }
}
