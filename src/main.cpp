
#include <engine/window.hpp>
#include <engine/model.hpp>
#include <engine/audio.hpp>

using namespace houseofatmos::engine;

static const i64 resolution = 360;

struct TestScene: Scene {

    static inline Model::LoadArgs HOUSE_MODEL = {
        "res/house.gltf", {
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
        this->load(Model::Loader(HOUSE_MODEL));
        this->load(Shader::Loader(MODEL_SHADER));
    }

    void update(const Window& window) override {}
    
    void render(const Window& window) override {
        this->time += window.delta_time();
        
        Model& house_model = this->get<Model>(HOUSE_MODEL);
        auto [house_mesh, house_texture, house_skeleton] = house_model.mesh("house");
        const Animation& animation = house_model.animation("door");
    
        Shader& model_shader = this->get<Shader>(MODEL_SHADER);
        model_shader.set_uniform("u_model", Mat<4>());
        model_shader.set_uniform("u_view", Mat<4>::look_at(
            Vec<3>(5, 5, 5), // camera position
            Vec<3>(0, 0, 0), // look at the origin
            Vec<3>(0, 1, 0) // up is along the positive Y axis
        ));
        model_shader.set_uniform("u_projection", Mat<4>::perspective(
            pi / 2.0, target.width(), target.height(), 0.1, 1000.0
        ));
        model_shader.set_uniform("u_joint_transf", animation.compute_transforms(
            *house_skeleton, fabs(sin(this->time)) * animation.length()
        ));
        model_shader.set_uniform("u_texture", house_texture);

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
        house_mesh.render(model_shader, target);
        window.show_texture(target);
    }

};

int main(int argc, char** argv) {
    auto window = Window(1280, 720, "House of Atmos");
    window.set_scene(std::unique_ptr<TestScene>(new TestScene()));
    window.start();
}
