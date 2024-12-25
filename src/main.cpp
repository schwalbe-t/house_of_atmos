
#include <engine/window.hpp>
#include <engine/model.hpp>
#include <engine/audio.hpp>

using namespace houseofatmos::engine;

static const i64 resolution = 360;

struct TestScene: Scene {

    static inline Model::LoadArgs MODEL = {
        "res/player.gltf", {
            { Model::Position, { Mesh::F32, 3 } }, 
            { Model::UvMapping, { Mesh::F32, 2 } }, 
            { Model::Normal, { Mesh::F32, 3 } },
            { Model::Joints, { Mesh::U8, 4 } },
            { Model::Weights, { Mesh::F32, 4 } }
        } 
    };

    static inline Shader::LoadArgs MODEL_SHADER = {
        "res/model_vert.glsl", "res/model_frag.glsl"
    };

    Texture target = Texture(100, 100);
    f64 time = 0;

    TestScene() {
        this->load(Model::Loader(MODEL));
        this->load(Shader::Loader(MODEL_SHADER));
    }

    void update(const Window& window) override {}
    
    void render(const Window& window) override {
        this->time += window.delta_time();
        
        Model& model = this->get<Model>(MODEL);
    
        Shader& shader = this->get<Shader>(MODEL_SHADER);
        shader.set_uniform("u_view", Mat<4>::look_at(
            Vec<3>(5, 5, 5), // camera position
            Vec<3>(0, 0, 0), // look at the origin
            Vec<3>(0, 1, 0) // up is along the positive Y axis
        ));
        shader.set_uniform("u_projection", Mat<4>::perspective(
            pi / 2.0, // 90 deg FOV
            target.width(), target.height(), // aspect ratio
            0.1, 1000.0 // include if between 0.1 and 1000 units away
        ));
        shader.set_uniform("u_light_pos", Vec<3>(100, 100, 100));
        shader.set_uniform("u_ambient_light", 0.6);

        Texture& target = this->target;
        if(window.height() < resolution) {
            target.resize_fast(window.width(), window.height());
        } else {
            f64 ratio = (f64) resolution / window.height();
            i64 width = ceil(window.width() * ratio);
            target.resize_fast(width, resolution);
        }
        target.clear_color(Vec<4>(0.1, 0.1, 0.1, 1.0));
        target.clear_depth(INFINITY);

        shader.set_uniform("u_model_transf", Mat<4>::rotate_y(this->time));
        auto animation = model.animation("floss");
        model.render_all_animated(
            shader, target, 
            animation, fmod(this->time, animation.length()),
            "u_joint_transf", "u_local_transf", "u_texture"
        );
        // model.render_all(
        //     shader, target, "u_local_transf", "u_texture", "u_joint_transf"
        // );

        window.show_texture(target);
    }

};

int main(int argc, char** argv) {
    auto window = Window(1280, 720, "House of Atmos");
    window.set_scene(std::unique_ptr<TestScene>(new TestScene()));
    window.start();
}
