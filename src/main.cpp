
#include "engine/engine.hpp"
#include "engine/resources.hpp"

namespace engine = houseofatmos::engine;
namespace rendering = houseofatmos::engine::rendering;
namespace resources = houseofatmos::engine::resources;
namespace animation = houseofatmos::engine::animation;
using namespace houseofatmos::engine::math;


// struct ModelShader: engine::rendering::Shader<engine::resources::ModelVertex, ModelShader> {
//     Mat<4> projection;
//     Mat<4> view;
//     Mat<4> model;
//     const engine::rendering::Surface* tex;

//     ModelShader() {}

//     Vec<4> vertex(engine::resources::ModelVertex vertex) override {
//         this->uv = vertex.uv;
//         return this->projection * this->view * this->model 
//             * vertex.pos.with(1.0);
//     }

//     Vec<2> uv;

//     Vec<4> fragment() override {
//         this->interpolate(&this->uv);
//         return this->tex->sample(uv);
//     }
// };

struct RiggedModelShader: rendering::Shader<resources::RiggedModelVertex, RiggedModelShader> {
    Mat<4> projection;
    Mat<4> view;
    Mat<4> model;
    Mat<4> local;
    const std::vector<resources::RiggedModelBone>* bones;
    const rendering::Surface* texture;

    Vec<4> vertex(resources::RiggedModelVertex vertex) override {
        this->uv = vertex.uv;
        Vec<4> pos;
        for(size_t i = 0; i < 4; i += 1) {
            pos += this->bones->at(vertex.joints[i]).anim_transform 
                * vertex.pos.with(1.0) * vertex.weights[i];
        }
        return this->projection * this->view * this->model * this->local * pos;
    }

    Vec<2> uv;

    Vec<4> fragment() override {
        this->interpolate(&this->uv);
        return this->texture->sample(uv);
    }
};

int main() {
    engine::init("House of Atmos", 1200, 800, 60);
    auto model = engine::resources::read_gltf_model("res/player.gltf");
    animation::Animation floss = model.animations["floss"];
    auto main_buffer = rendering::Surface(1200, 800);
    auto sub_buffer = rendering::Surface(1200, 800);
    auto model_shader = RiggedModelShader();
    model_shader.projection = Mat<4>::perspective(PI / 2.0, sub_buffer.width, sub_buffer.height, 0.1, 1000.0);
    model_shader.view = Mat<4>::look_at(Vec<3>(0, 5, 10), Vec<3>(0, 5, 0), Vec<3>(0, 1, 0));
    model_shader.bones = &model.bones;
    double anim_timer = 0.0;
    while(engine::is_running()) {
        anim_timer = fmod(anim_timer + GetFrameTime(), floss.length);
        floss.compute_transforms(model.bones, model.root_bone_i, anim_timer);
        main_buffer.clear();
        sub_buffer.clear();
        model.draw(sub_buffer, model_shader);
        main_buffer.blit_buffer(sub_buffer, 0, 0, 1200, 800);
        engine::display_buffer(&main_buffer);
    }
    engine::stop();
}























