
#pragma once

#include "../player.hpp"
#include "../interactable.hpp"
#include "../toasts.hpp"
#include "terrain.hpp"

namespace houseofatmos::outside {

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
            scene.load(engine::Model::Loader(PersonalHorse::horse_model));
            scene.load(engine::Texture::Loader(PersonalHorse::horse_texture));
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

        Player* player;
        Interactables* interactables;
        engine::Scene* scene = nullptr;
        
        std::shared_ptr<Interactable> interactable = nullptr;
        
        PersonalHorse(
            Vec<3> pos, Player* player, Interactables* interactables
        ) {
            this->player = player;
            this->interactables = interactables;
            this->set_free(pos);
        }

        PersonalHorse(
            const Serialized& serialized,
            Player* player, Interactables* interactables
        ) {
            this->player = player;
            this->interactables = interactables;
            this->set_free(serialized.pos);
            this->angle = serialized.angle;
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
            const Terrain& terrain, Toasts& toasts
        );

        void render(
            engine::Scene& scene, const engine::Window& window, 
            const Renderer& renderer
        );

        Serialized serialize() const;

    };

}