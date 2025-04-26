
#pragma once

#include "renderer.hpp"
#include <algorithm>

namespace houseofatmos {

    using namespace houseofatmos::engine::math;


    struct Particle {

        struct Type {
            engine::Texture::LoadArgs texture;
            Vec<2> offset_tex;
            Vec<2> size_tex;
            f64 duration;
            Vec<3> (*position_of)(
                const Particle& particle, const engine::Window& window
            );
            Vec<2> (*size_of)(
                const Particle& particle, const engine::Window& window
            );

            Particle at(Vec<3> position) const {
                return Particle(this, position, 0.0);
            }
        };

        const Type* type;
        Vec<3> start_pos;
        f64 age;

    };


    struct ParticleManager {

        static const inline engine::Shader::LoadArgs shader_args = {
            "res/shaders/particle_vert.glsl", "res/shaders/particle_frag.glsl"
        };

        static void load_shaders(engine::Scene& scene) {
            scene.load(ParticleManager::shader_args);
        }

        static const inline size_t max_inst_c = 128;

        private:
        engine::Mesh billboard = engine::Mesh {
            engine::Mesh::Attrib(engine::Mesh::F32, 2)
        };
        std::unordered_map<const Particle::Type*, std::vector<Particle>> 
            particles;
        std::vector<Vec<3>> positions;
        std::vector<Vec<2>> sizes;

        public:
        ParticleManager() {
            this->billboard.start_vertex();
                this->billboard.put_f32({ 0, 1 });
            u16 tl = this->billboard.complete_vertex();
            this->billboard.start_vertex();
                this->billboard.put_f32({ 1, 1 });
            u16 tr = this->billboard.complete_vertex();
            this->billboard.start_vertex();
                this->billboard.put_f32({ 0, 0 });
            u16 bl = this->billboard.complete_vertex();
            this->billboard.start_vertex();
                this->billboard.put_f32({ 1, 0 });
            u16 br = this->billboard.complete_vertex();
            this->billboard.add_element(tl, bl, br);
            this->billboard.add_element(br, tr, tl);
            this->billboard.submit();
        }

        void add(Particle particle) {
            this->particles[particle.type].push_back(particle);
        }

        private:
        static void update_particles(
            std::vector<Particle>& particles, const engine::Window& window
        ) {
            // update age of particles
            for(Particle& particle: particles) {
                particle.age += window.delta_time();
            }
            // remove particles if they are too old
            auto new_particles_end = std::remove_if(
                particles.begin(), particles.end(),
                [](const auto& p) { return p.age >= p.type->duration; }
            );
            particles.erase(new_particles_end, particles.end());
        }

        static void collect_values(
            const std::vector<Particle>& particles, 
            const engine::Window& window,
            std::vector<Vec<3>>& positions, std::vector<Vec<2>>& sizes
        ) {
            for(const Particle& particle: particles) {
                Vec<3> position = particle.type->position_of(particle, window);
                positions.push_back(position);
                Vec<2> size = particle.type->size_of(particle, window);
                sizes.push_back(size);
            }
        } 

        public:
        void render(
            Renderer& renderer, 
            engine::Scene& scene, const engine::Window& window,
            engine::DepthTesting depth_testing = engine::DepthTesting::Enabled
        ) {
            engine::Shader& shader = scene.get(ParticleManager::shader_args);
            shader.set_uniform("u_view_proj", renderer.compute_view_proj());
            Camera& cam = renderer.camera;
            Vec<3> cam_forward = (cam.position - cam.look_at).normalized();
            Vec<3> cam_right = cam.up.cross(cam_forward).normalized();
            Vec<3> cam_true_up = cam_forward.cross(cam_right).normalized();
            shader.set_uniform("u_camera_right", cam_right);
            shader.set_uniform("u_camera_up", cam_true_up);
            shader.set_uniform("u_camera_forward", renderer.camera.look_at);
            renderer.set_fog_uniforms(shader);
            renderer.set_shadow_uniforms(shader);
            for(auto& [type, instances]: this->particles) {
                ParticleManager::update_particles(instances, window);
                this->positions.clear();
                this->sizes.clear();
                ParticleManager::collect_values(
                    instances, window, this->positions, this->sizes
                );
                engine::Texture& tex = scene.get(type->texture);
                Vec<2> tex_size = Vec<2>(tex.width(), tex.height());
                shader.set_uniform("u_texture", tex);
                shader.set_uniform("u_uv_size", type->size_tex / tex_size);
                f64 uv_o_u = type->offset_tex.x() / tex_size.x();
                f64 uv_o_v = (
                    tex_size.y() - type->offset_tex.y() - type->size_tex.y()
                ) / tex_size.y();
                shader.set_uniform("u_uv_offset", Vec<2>(uv_o_u, uv_o_v));
                std::span<const Vec<3>> pos = this->positions;
                std::span<const Vec<2>> size = this->sizes;
                for(size_t o = 0; o < this->positions.size();) {
                    size_t r = this->positions.size() - o;
                    size_t c = std::min(r, ParticleManager::max_inst_c);
                    shader.set_uniform("u_size", size.subspan(o, c));
                    shader.set_uniform("u_w_center_pos", pos.subspan(o, c));
                    this->billboard.render(
                        shader, renderer.output().as_target(), c,
                        engine::FaceCulling::Disabled, depth_testing
                    );
                    o += c;
                }
            }
        }

    };


    struct ParticleSpawner {

        struct Type {
            f64 attempt_period;
            f64 spawn_chance;
            void (*handler)(
                ParticleSpawner& spawner, ParticleManager& particles
            );

            ParticleSpawner at(Vec<3> position, StatefulRNG& rng) const {
                return ParticleSpawner(this, position, rng);
            }
        };

        const Type* type;
        Vec<3> position;
        StatefulRNG rng;
        f64 timer = 0.0;

        ParticleSpawner(const Type* type, Vec<3> position, StatefulRNG& rng) {
            this->type = type;
            this->position = position;
            this->rng = rng;
            this->timer = rng.next_f64() * this->type->attempt_period;
        }

        void spawn(const engine::Window& window, ParticleManager& particles) {
            if(this->timer >= this->type->attempt_period) {
                if(this->rng.next_f64() < this->type->spawn_chance) {
                    this->type->handler(*this, particles);
                }
                this->timer = fmod(this->timer, this->type->attempt_period);
            }
            this->timer += window.delta_time();
        }

    };

}
