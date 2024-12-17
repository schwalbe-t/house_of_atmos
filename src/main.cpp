
#include <engine/window.hpp>
#include <engine/scene.hpp>

using namespace houseofatmos::engine;

static const f64 resolution = 100;

struct TestScene: Scene {

    static inline Model::LoadArgs PLAYER_MODEL = {
        "res/player.gltf", {
            { Model::Position, { Mesh::F32, 3 } }, 
            // { Model::UvMapping, { Mesh::F32, 2 } }, 
            // { Model::Normal, { Mesh::F32, 3 } }
        } 
    };

    static inline Shader::LoadArgs MODEL_SHADER = {
        "res/model_vert.glsl", "res/model_frag.glsl"
    };

    Texture target = Texture(100, 100);

    TestScene() {
        this->load(Model::Loader(PLAYER_MODEL));
        this->load(Shader::Loader(MODEL_SHADER));
    }

    void update(const Window& window) override {}
    
    void render(const Window& window) override {
        Shader& model_shader = this->get<Shader>(MODEL_SHADER);
        Model& player_model = this->get<Model>(PLAYER_MODEL);
        Texture& target = this->target;
        target.resize_fast(window.width(), window.height());
        target.clear_color(Vec<4>(0.5, 0.5, 0.5, 1.0));
        target.clear_depth(1.0);
        model_shader.set_uniform("u_model", Mat<4>());
        model_shader.set_uniform("u_view", Mat<4>());
        model_shader.set_uniform("u_projection", Mat<4>());
        // model_shader.set_uniform("u_view", Mat<4>::look_at(
        //     Vec<3>(0, 0, 10), // camera position
        //     Vec<3>(0, 0, 0 ), // look at the origin
        //     Vec<3>(0, 1, 0 ) // up is along the positive Y axis
        // ));
        // model_shader.set_uniform("u_projection", Mat<4>::perspective(
        //     pi / 2.0, window.width(), window.height(), 0.1, 1000.0
        // ));
        player_model.render(model_shader, target, "u_texture");
        window.show_texture(target);
    }

};

int main(int argc, char** argv) {
    auto window = Window(1280, 720, "House of Atmos");
    window.set_scene(std::shared_ptr<Scene>(new TestScene()));
    window.start();
}
