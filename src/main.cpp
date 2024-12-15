
#include <engine/window.hpp>
#include <engine/scene.hpp>

using namespace houseofatmos::engine;

static const f64 resolution = 100;

struct TestScene: Scene {

    static inline Shader::LoadArgs SHADER = { "res/test_vert.glsl", "res/test_frag.glsl" };

    std::vector<Vec<4>> colors = {
        Vec<4>(1.0, 0.0, 0.0, 1.0),
        Vec<4>(0.0, 1.0, 0.0, 1.0),
        Vec<4>(0.0, 0.0, 1.0, 1.0)
    };
    Mesh mesh = Mesh { 3, 3 };
    Texture target = Texture(100, 100);

    TestScene() {
        this->mesh.add_element(
            this->mesh.add_vertex({ -0.5,  0.5, 0,   1, 0, 0 }),
            this->mesh.add_vertex({  0.5,  0.5, 0,   0, 1, 0 }),
            this->mesh.add_vertex({  0.0, -0.5, 0,   0, 0, 1 })
        );
        this->mesh.submit();
        this->load(Shader::Loader(SHADER));
    }

    void update(const Window& window) override {
           
    }
    
    void render(const Window& window) override {
        Shader& shader = this->get<Shader>(SHADER);
        if(resolution < window.height()) {
            this->target.resize_fast(ceil(resolution * window.width() / window.height()), resolution);
        } else {
            this->target.resize_fast(window.width(), window.height());
        }
        this->target.clear_color(Vec<4>(1.0, 1.0, 1.0, 1.0));
        this->target.clear_depth(INFINITY);
        shader.set_uniform("u_colors", this->colors);
        this->mesh.render(shader, this->target);
        window.show_texture(this->target);
    }

};

int main(int argc, char** argv) {
    auto window = Window(1280, 720, "House of Atmos");
    window.set_scene(std::shared_ptr<Scene>(new TestScene()));
    window.start();
}
