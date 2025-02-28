
#include "dialogue.hpp"
#include "ui_const.hpp"

namespace houseofatmos {

    void DialogueManager::stop_voice_sounds(engine::Scene& scene) const {
        if(this->queued.size() == 0) { return; }
        const Dialogue::Voice& voice = *this->queued[0].voice;
        for(const auto& [c, sound]: voice.sounds) {
            scene.get<engine::Sound>(sound).stop();
        }
        scene.get<engine::Sound>(voice.default_sound).stop();
    }



    ui::Element DialogueManager::create_container() {
        ui::Element container = ui::Element()
            .with_handle(&this->container)
            .as_hidden(true)
            .with_size(0, 0, ui::size::units_with_children)
            .with_pos(0.5, 0.95, ui::position::window_fract)
            .with_background(&ui_background::scroll_horizontal)
            .as_movable();
        container.children.push_back(ui::Element()
            .with_handle(&this->lines)
            .with_size(150, 50, ui::size::units)
            .with_text("", &ui_font::dark)
            .with_padding(3.0)
            .as_movable()
        );
        return container;
    }

    void DialogueManager::set_displayed_lines(std::string text) const {
        this->lines->text = std::move(text);
    }



    // UTF-8:
    // size=1: 0yyyzzzz
    // size=2: 110xxxyy 0yyzzzz
    // size=3: 1110wwww 10xxxxyy 10yyzzzz
    // size=4: 11110uvv 10vvwwww 10xxxxyy 10yyzzzz

    // UTF-32:
    //         00000000 000uvvvv wwwwxxxx yyyyzzzz

    static size_t utf8_char_size(std::string_view input) {
        if(input.size() == 0) { return 0; }
        u8 first = (u8) input[0];
        if((first & 0b10000000) == 0b00000000) { return 1; }
        if((first & 0b11100000) == 0b11000000) { return 2; }
        if((first & 0b11110000) == 0b11100000) { return 3; }
        if((first & 0b11111000) == 0b11110000) { return 4; }
        return 1;
    }

    static char32_t utf8_char_to_char32(std::string_view input) {
        const u8* bytes = (const u8*) input.data();
        char32_t c = 0;
        switch(input.size()) {
            case 1: c = bytes[0] & 0b01111111; break;
            case 2: c = bytes[0] & 0b00011111; break;
            case 3: c = bytes[0] & 0b00001111; break;
            case 4: c = bytes[0] & 0b00000111; break;
            default: return 0xFFFD; // invalid sequence 
        }
        for(size_t i = 1; i < input.size(); i += 1) {
            c = (c << 6) | (bytes[i] & 0b00111111);
        }
        return c;
    }

    void DialogueManager::skip(engine::Scene& scene) {
        if(this->is_empty()) { return; }
        this->stop_voice_sounds(scene);
        this->queued[0].handler();
        this->queued.erase(this->queued.begin());
        this->current_offset = 0;
        this->char_timer = 0.0;
        this->set_displayed_lines("");
        if(this->queued.size() == 0) {
            this->container->hidden = true;
        }
    }

    void DialogueManager::advance_to_next_char(engine::Scene& scene) {
        const Dialogue& dialogue = this->queued[0];
        std::string remaining = dialogue.text.substr(this->current_offset);
        size_t char_size = utf8_char_size(remaining);
        std::string next_char_s = remaining.substr(0, char_size);
        char32_t next_char = utf8_char_to_char32(next_char_s);
        engine::Sound& sound = scene
            .get<engine::Sound>(dialogue.voice->get_sound_of(next_char));
        sound.set_base_pitch(dialogue.pitch);
        sound.play();
        this->current_offset += char_size;
        std::string displayed = dialogue.text.substr(0, this->current_offset);
        const engine::Localization local = scene
            .get<engine::Localization>(this->local_ref);
        this->set_displayed_lines("[" + dialogue.name + "]" 
            + "\n\n" + displayed 
            + (this->waiting()
                ? "\n\n" + local.text("dialogue_press_to_continue") : ""
            )
        );
    }

    void DialogueManager::update(
        engine::Scene& scene, const engine::Window& window, 
        const Vec<3>& observer
    ) {
        bool has_work = this->container != nullptr
            && this->lines != nullptr
            && this->queued.size() > 0;
        if(!has_work) { return; }
        const Dialogue& dialogue = this->queued[0];
        this->container->hidden = false;
        if(this->current_offset < dialogue.text.size()) {
            this->char_timer += window.delta_time();
            f64 char_duration = dialogue.voice->base_char_duration
                / dialogue.speed;
            while(this->char_timer > char_duration) {
                this->advance_to_next_char(scene);
                this->char_timer -= char_duration;
            }
        }
        f64 distance = (observer - dialogue.origin).len();
        bool requested_skip = window.was_pressed(engine::Button::Left)
            || window.was_pressed(engine::Key::Enter)
            || window.was_pressed(engine::Key::Space)
            || window.was_pressed(engine::Key::E);
        bool skipped = (this->waiting() && requested_skip)
            || distance > dialogue.max_distance;
        if(skipped) {
            this->skip(scene);
        }
    }

}