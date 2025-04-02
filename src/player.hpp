
#pragma once

#include <engine/arena.hpp>
#include <engine/math.hpp>
#include <engine/model.hpp>
#include <engine/audio.hpp>
#include "settings.hpp"
#include "collider.hpp"
#include "renderer.hpp"
#include "human.hpp"

namespace houseofatmos {

    using namespace houseofatmos::engine::math;

    
    struct Player {
    
        struct Rideable {
            Vec<3> position;
            f64 angle;
            u64 animation_id;
        };

        struct Serialized {
            Vec<3> position;
            f64 angle;
        };


        public:
        const Rideable* riding = nullptr;
        Character character;
        Vec<3> next_step;
        Vec<3> confirmed_step;
        bool in_water = false;
        bool is_riding = false;

        Player(const Settings& settings)
            : character(Character(
                &human::male, &human::count,
                { 0, 0, 0 }, (u64) human::Animation::Stand,
                &settings
            )) {
            this->character.speaker.range = 5.0;
        }

        Player(const Settings& settings, const Serialized& serialized)
            : character(Character(
            &human::male, &human::count,
            serialized.position, (u64) human::Animation::Stand,
            &settings
        )) {
            this->character.angle = serialized.angle;
            this->character.speaker.range = 5.0;
        }

        void update(const engine::Window& window);
        Vec<3> next_x() {
            return this->character.position + this->next_step * Vec<3>(1, 0, 0); 
        }
        Vec<3> next_z() { 
            return this->character.position + this->next_step * Vec<3>(0, 0, 1); 
        }
        void proceed_x() { this->confirmed_step.x() += this->next_step.x(); }
        void proceed_z() { this->confirmed_step.z() += this->next_step.z(); }
        void apply_confirmed_step(engine::Scene& scene, const engine::Window& window);
        void render(
            engine::Scene& scene, const engine::Window& window, 
            Renderer& renderer
        );

        Serialized serialize() const;

    };

}