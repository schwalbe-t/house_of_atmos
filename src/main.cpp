
#include <engine/window.hpp>
#include <engine/scene.hpp>

using namespace houseofatmos::engine;

static const f64 resolution = 100;

struct TestScene: Scene {

    static inline std::string_view PLAYER = "res/player.png";

    // Shader shader = Shader(
    //     "#version 130 \n"
    //     "in vec3 v_pos; \n"
    //     "in vec3 v_color; \n"
    //     "out vec3 f_color; \n"
    //     "void main() { \n"
    //     "    gl_Position = vec4(v_pos, 1.0); \n"
    //     "    f_color = v_color; \n"
    //     "}",
    //     "#version 130 \n"
    //     "in vec3 f_color; \n"
    //     "uniform vec4 u_colors[3]; \n"
    //     "void main() { \n"
    //     "    gl_FragColor = u_colors[0] * round(f_color.x * 2) / 2 + u_colors[1] * round(f_color.y * 2) / 2 + u_colors[2] * round(f_color.z * 2) / 2; \n"
    //     "}"
    // );
    // std::vector<Vec<4>> colors = {
    //     Vec<4>(1.0, 0.0, 0.0, 1.0),
    //     Vec<4>(0.0, 1.0, 0.0, 1.0),
    //     Vec<4>(0.0, 0.0, 1.0, 1.0)
    // };
    // Mesh mesh = Mesh { 3, 3 };
    // Texture target = Texture(100, 100);

    TestScene() {
        // this->mesh.add_element(
        //     this->mesh.add_vertex({ -0.5,  0.5, 0,   1, 0, 0 }),
        //     this->mesh.add_vertex({  0.5,  0.5, 0,   0, 1, 0 }),
        //     this->mesh.add_vertex({  0.0, -0.5, 0,   0, 0, 1 })
        // );
        // this->mesh.submit();
        this->load(Resource<Texture>(PLAYER));
    }

    void update(const Window& window) override {
           
    }
    
    void render(const Window& window) override {
        // if(resolution < window.height()) {
        //     this->target.resize_fast(ceil(resolution * window.width() / window.height()), resolution);
        // } else {
        //     this->target.resize_fast(window.width(), window.height());
        // }
        // this->target.clear_color(Vec<4>(1.0, 1.0, 1.0, 1.0));
        // this->target.clear_depth(INFINITY);
        // this->shader.set_uniform("u_colors", this->colors);
        // this->mesh.render(this->shader, this->target);
        // window.show_texture(this->target);
        window.show_texture(this->get<Texture>(PLAYER));
    }

};

int main(int argc, char** argv) {
    auto window = Window(1280, 720, "House of Atmos");
    window.set_scene(std::shared_ptr<Scene>(new TestScene()));
    window.start();
}
