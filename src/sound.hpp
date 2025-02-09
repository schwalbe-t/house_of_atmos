
#pragma once

#include <engine/audio.hpp>

namespace houseofatmos {

    struct Sound {
        struct LoadArgs {
            std::string path;
            f64 base_pitch;
            f64 pitch_variation;

            std::string identifier() const { 
                return this->path 
                    + "|||" + std::to_string(this->base_pitch)
                    + "|||" + std::to_string(this->pitch_variation);
            }
            std::string pretty_identifier() const {
                return "Sound[" + std::to_string(this->base_pitch)
                    + " +/- " + std::to_string(this->pitch_variation) + "]"
                    "@'" + this->path + "'";
            }
        };
        using Loader = engine::Resource<Sound, LoadArgs>;

        private:
        engine::Audio player;
        f64 curr_base_pitch;
        f64 curr_pitch_variation;

        public:
        Sound(
            engine::Audio&& audio, f64 base_pitch, f64 pitch_variation
        ): player(std::move(audio)) {
            this->curr_base_pitch = base_pitch;
            this->curr_pitch_variation = pitch_variation;
            this->randomize_pitch();
        }

        static Sound from_resource(const LoadArgs& arg);

        void randomize_pitch();

        void play() {
            this->randomize_pitch();
            this->player.play();
        }

        void stop() { this->player.stop(); }
        void set_gain(f64 value) { this->player.set_gain(value); }

        void set_base_pitch(f64 base_pitch) {
            this->curr_base_pitch = base_pitch;
            this->randomize_pitch();
        }

        void set_pitch_variation(f64 pitch_variation) {
            this->curr_pitch_variation = pitch_variation;
            this->randomize_pitch();
        }

        f64 base_pitch() const { return this->curr_base_pitch; }
        f64 pitch_variation() const { return this->curr_pitch_variation; }
        bool is_playing() const { return this->is_playing(); }
        const engine::Audio& audio() const { return this->player; }

    };

}