
#include <engine/window.hpp>
#include <engine/scene.hpp>

using namespace houseofatmos::engine;

static const f64 resolution = 100;

struct TestScene: Scene {

    static inline Shader::LoadArgs SHADER = {
        "res/test_vert.glsl", "res/test_frag.glsl"
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

    void update(const Window& window) override {}
    
    void render(const Window& window) override {
        Shader& shader = this->get<Shader>(SHADER);
        Texture& target = this->target;
        target.resize_fast(window.width(), window.height());
        target.clear_color(Vec<4>(1.0, 1.0, 1.0, 1.0));
        target.clear_depth(INFINITY);
        this->mesh.render(shader, target);
        window.show_texture(target);
    }

};

int main(int argc, char** argv) {
    auto window = Window(1280, 720, "House of Atmos");
    window.set_scene(std::shared_ptr<Scene>(new TestScene()));
    window.start();
}
