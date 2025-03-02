
#pragma once

#include "scene.hpp"
#include "nums.hpp"
#include "rng.hpp"
#include <vector>

namespace houseofatmos::engine {

    struct Audio {
        struct LoadArgs {
            std::string_view path;

            std::string identifier() const { return std::string(path); }
            std::string pretty_identifier() const {
                return "Audio@'" + std::string(path) + "'";
            }   
        };
        using Loader = Resource<Audio, LoadArgs>;

        private:
        u8 data_channels;
        u64 data_sample_rate;
        u64 buffer_id;
        u64 source_id;
        bool moved;
        
        Audio() {}


        public:
        Audio(const Audio& other) = delete;
        Audio(Audio&& other) noexcept;
        static Audio from_resource(const LoadArgs& arg);
        Audio& operator=(const Audio& other) = delete;
        Audio& operator=(Audio&& other) noexcept;
        ~Audio();

        u8 channels() const { return this->data_channels; }
        u64 sample_rate() const { return this->data_sample_rate; }
        
        void play();
        void stop();
        void set_pitch(f64 value);
        void set_gain(f64 value);
        
        bool is_playing() const;
    };


    struct Sound {
        struct LoadArgs {
            std::string_view path;
            f64 base_pitch;
            f64 pitch_variation;

            std::string identifier() const { 
                return std::string(this->path) 
                    + "|||" + std::to_string(this->base_pitch)
                    + "|||" + std::to_string(this->pitch_variation);
            }
            std::string pretty_identifier() const {
                return "Sound[" + std::to_string(this->base_pitch)
                    + " +/- " + std::to_string(this->pitch_variation) + "]"
                    "@'" + std::string(this->path) + "'";
            }
        };
        using Loader = Resource<Sound, LoadArgs>;

        private:
        Audio player;
        f64 curr_base_pitch;
        f64 curr_pitch_variation;

        public:
        Sound(
            Audio&& audio, f64 base_pitch, f64 pitch_variation
        ): player(std::move(audio)) {
            this->curr_base_pitch = base_pitch;
            this->curr_pitch_variation = pitch_variation;
            this->randomize_pitch();
        }

        static Sound from_resource(const LoadArgs& args);

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
        bool is_playing() const { return this->player.is_playing(); }
        const Audio& audio() const { return this->player; }

    };


    struct Soundtrack {

        static inline const bool repetition_allowed = true;
        static inline const bool no_repetition = false; 

        struct LoadArgs {
            std::span<const std::string_view> track_sources;
            bool allow_repetition;

            std::string identifier() const {  return this->pretty_identifier(); }

            std::string pretty_identifier() const {
                std::string result = "Soundtrack[" 
                    + std::string(this->allow_repetition
                        ? "repetition allowed"
                        : "no repetition"
                    )
                    + "]@(";
                bool had_path = false;
                for(std::string_view path: this->track_sources) {
                    if(had_path) { result += ", "; }
                    result += "'" + std::string(path) + "'";
                    had_path = true;
                }
                return result + ")";
            }
        };
        using Loader = Resource<Soundtrack, LoadArgs>;

        private:
        std::vector<Audio> tracks;
        bool allow_repetition;
        math::StatefulRNG rng;
        u64 last_played_idx = UINT64_MAX;

        bool any_is_playing() {
            for(const Audio& track: this->tracks) {
                if(track.is_playing()) { return true; }
            }
            return false;
        }

        public:
        Soundtrack(
            std::vector<Audio>&& tracks, 
            bool allow_repetition = Soundtrack::no_repetition,
            math::StatefulRNG rng = math::StatefulRNG()
        ): tracks(std::move(tracks)) {
            this->allow_repetition = allow_repetition;
            this->rng = rng;
        }

        static Soundtrack from_resource(const LoadArgs& args) {
            std::vector<Audio> tracks;
            tracks.reserve(args.track_sources.size());
            for(std::string_view track_source: args.track_sources) {
                auto track = Audio::from_resource(
                    (Audio::LoadArgs) { track_source }
                );
                tracks.push_back(std::move(track));
            }
            return Soundtrack(std::move(tracks), args.allow_repetition);
        }

        void set_gain(f64 value) {
            for(Audio& track: this->tracks) {
                track.set_gain(value);
            }
        }

        void update() {
            if(this->any_is_playing()) { return; }
            for(;;) {
                u64 track_idx = (u64) (
                    this->rng.next_f64() * (f64) this->tracks.size()
                ) % this->tracks.size();
                bool is_valid = this->allow_repetition
                    || track_idx != this->last_played_idx;
                if(!is_valid) { continue; }
                this->last_played_idx = track_idx;
                this->tracks[track_idx].play();
                break;
            }
        }

    };

}