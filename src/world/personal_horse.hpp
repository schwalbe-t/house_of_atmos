
#pragma once

#include "../player.hpp"
#include "../interactable.hpp"
#include "../toasts.hpp"
#include "../settings.hpp"
#include "terrain.hpp"

namespace houseofatmos::world {

    struct PersonalHorse {

        static inline const engine::Model::LoadArgs horse_model = {
            "res/entities/horse.glb", Renderer::model_attribs,
            engine::FaceCulling::Disabled
        };

        static inline const engine::Texture::LoadArgs horse_texture = {
            "res/entities/horse_black.png",
            engine::Texture::vertical_mirror // (because used as GLTF replacement texture)
        };

        static void load_resources(engine::Scene& scene) {
            scene.load(PersonalHorse::horse_model);
            scene.load(PersonalHorse::horse_texture);
        }


        struct Serialized {
            Vec<3> pos;
            f64 angle;
        };

        enum struct State {
            Ridden, Idle, Called
        };


        State state = State::Idle;
        Vec<3> pos;
        
        Vec<3> last_pos = Vec<3>(0.0, 0.0, 0.0);
        f64 angle = 0.0;
        bool is_moving = false;

        Player* player = nullptr;
        engine::Scene* scene = nullptr;
        
        engine::Speaker speaker = engine::Speaker(
            engine::Speaker::Space::World, 10.0
        );
        std::shared_ptr<Interactable> interactable = nullptr;
        
        PersonalHorse(const Settings& settings, Vec<3> pos = Vec<3>(0, 0, 0)) {
            this->set_free(pos);
            this->speaker.volume = settings.sfx_volume;
        }

        PersonalHorse(const Settings& settings, const Serialized& serialized) {
            this->set_free(serialized.pos);
            this->angle = serialized.angle;
            this->speaker.volume = settings.sfx_volume;
        }

        void set_free(Vec<3> at_pos);
        void set_called();
        void set_ridden();

        const Vec<3>& position() const {
            switch(this->state) {
                case State::Idle: 
                case State::Called: 
                    return this->pos;
                case State::Ridden: 
                    return this->player->character.position;
                default: 
                engine::error(
                    "Unhandled 'PersonalHorse::State' in 'PersonalHorse::position'"
                );
            }
        }

        private: 
        void update_heading();

        public:

        void update(
            engine::Scene& scene, const engine::Window& window, 
            const Terrain& terrain, Player& player, 
            Interactables& interactables, Toasts& toasts
        );

        void render(
            engine::Scene& scene, const engine::Window& window, 
            Renderer& renderer
        );

        Serialized serialize() const;

    };

}