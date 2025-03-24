
#pragma once

#include <engine/audio.hpp>
#include <engine/ui.hpp>
#include <engine/localization.hpp>
#include "settings.hpp"
#include <functional>

namespace houseofatmos {

    namespace ui = houseofatmos::engine::ui;
    using namespace houseofatmos::engine::math;


    struct Dialogue {

        struct Voice {
            std::unordered_map<char32_t, engine::Sound::LoadArgs> sounds;
            engine::Sound::LoadArgs default_sound;
            f64 base_char_duration;

            inline void load(engine::Scene& scene) const {
                for(const auto& [c, sound]: this->sounds) {
                    scene.load(sound);
                }
                scene.load(this->default_sound);
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
        Vec<3> origin;
        f64 max_distance;
        std::function<void ()> handler;

        Dialogue(
            std::string&& name,
            std::string&& text, 
            const Voice* voice, 
            f64 pitch, f64 speed,
            Vec<3> origin, f64 max_distance = 5.0,
            std::function<void ()>&& handler = [](){}
        ): name(std::move(name)), text(std::move(text)), 
            voice(voice), pitch(pitch), speed(speed), 
            origin(origin), max_distance(max_distance),
            handler(std::move(handler)) {}
    };

    struct DialogueManager {

        static inline const size_t speaker_pool_size = 2;

        private:
        std::array<engine::Speaker, speaker_pool_size> speakers;
        size_t next_speaker = 0;
        engine::Localization::LoadArgs local_ref;
        std::vector<Dialogue> queued;
        size_t current_offset = 0;
        f64 char_timer = 0.0;

        ui::Element* container = nullptr;
        ui::Element* lines = nullptr;

        void set_displayed_lines(std::string text) const;
        void advance_to_next_char(engine::Scene& scene);

        public:
        DialogueManager(const Settings& settings):
                local_ref(settings.localization()) {
            for(engine::Speaker& speaker: this->speakers) {
                speaker.volume = settings.sfx_volume;
            }
        }

        bool is_empty() const { return this->queued.size() == 0; }
        void say(Dialogue&& dialogue) { this->queued.push_back(dialogue); }
        bool waiting() const {
            if(this->queued.size() == 0) { return false; }
            return this->current_offset >= this->queued[0].text.size();
        }
        void skip();

        ui::Element create_container();

        void update(
            engine::Scene& scene, engine::Window& window,
            const Vec<3>& observer
        );

    };

}