
#pragma once

#include <engine/audio.hpp>
#include <engine/ui.hpp>
#include <engine/localization.hpp>
#include <functional>

namespace houseofatmos {

    namespace ui = houseofatmos::engine::ui;


    struct Dialogue {

        struct Voice {
            std::unordered_map<char32_t, engine::Sound::LoadArgs> sounds;
            engine::Sound::LoadArgs default_sound;
            f64 base_char_duration;

            inline void load(engine::Scene& scene) const {
                for(const auto& [c, sound]: this->sounds) {
                    scene.load(engine::Sound::Loader(sound));
                }
                scene.load(engine::Sound::Loader(this->default_sound));
            }

            inline void set_gain(engine::Scene& scene, f64 value) const {
                for(const auto& [c, sound]: this->sounds) {
                    scene.get<engine::Sound>(sound).set_gain(value);
                }
                scene.get<engine::Sound>(this->default_sound).set_gain(value);
            }

            inline const engine::Sound::LoadArgs& get_sound_of(char32_t c) const {
                auto specified = this->sounds.find(c);
                if(specified == this->sounds.end()) { 
                    return this->default_sound; 
                }
                return specified->second;
            }
        };


        std::string name;
        std::string text;
        const Voice* voice;
        f64 pitch;
        f64 speed;
        std::function<void ()> handler;

        Dialogue(
            std::string&& name,
            std::string&& text, 
            const Voice* voice, 
            f64 pitch = 1.0, f64 speed = 1.0,
            std::function<void ()>&& handler = [](){}
        ): name(std::move(name)), text(std::move(text)),
            voice(voice), pitch(pitch), speed(speed), 
            handler(std::move(handler)) {}
    };

    struct DialogueManager {

        private:
        std::vector<Dialogue> queued;
        size_t current_offset = 0;
        f64 char_timer = 0.0;

        ui::Element* container = nullptr;
        ui::Element* lines = nullptr;

        void stop_voice_sounds(engine::Scene& scene) const;
        void set_displayed_lines(std::string text) const;
        void advance_to_next_char(engine::Scene& scene);

        public:
        DialogueManager() {}

        void say(Dialogue&& dialogue) { this->queued.push_back(dialogue); }
        void skip(engine::Scene& scene);

        ui::Element create_container();

        void update(engine::Scene& scene, const engine::Window& window);

    };

}