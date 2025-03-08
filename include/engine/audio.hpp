
#pragma once

#include "scene.hpp"
#include "math.hpp"
#include "rng.hpp"
#include "util.hpp"
#include <vector>

namespace houseofatmos::engine {

    using namespace houseofatmos::engine::math;


    struct Volume {
        f64 gain = 1.0;
    };


    struct Audio {
        struct LoadArgs {
            using ResourceType = Audio;

            std::string_view path;
            LoadArgs(std::string_view path): path(path) {}

            std::string identifier() const { return std::string(path); }
            std::string pretty_identifier() const {
                return "Audio@'" + std::string(path) + "'";
            }   
        };

        private:
        static void destruct_buffer(const u64& buffer_id);

        util::Handle<u64, &destruct_buffer> buffer;
        u8 data_channels;
        u64 data_sample_rate;
        
        Audio() {}

        public:
        static Audio from_resource(const LoadArgs& arg);
        
        u8 channels() const { return this->data_channels; }
        u64 sample_rate() const { return this->data_sample_rate; }

        u64 internal_buffer_id() const { return *this->buffer; }
    };

    struct Sound {
        struct LoadArgs {
            using ResourceType = Sound;

            std::string_view path;
            f64 pitch_var;
            f64 base_pitch;
            LoadArgs(
                std::string_view path, f64 pitch_var, f64 base_pitch = 1.0
            ): path(path), base_pitch(base_pitch), pitch_var(pitch_var) {}

            std::string identifier() const { 
                return std::string(this->path) 
                    + "|||" + std::to_string(this->pitch_var)
                    + "|||" + std::to_string(this->base_pitch);
            }
            std::string pretty_identifier() const {
                return "Sound[" + std::to_string(this->base_pitch)
                    + " +/- " + std::to_string(this->pitch_var) + "]"
                    "@'" + std::string(this->path) + "'";
            }
        };

        Audio audio;
        f64 base_pitch;
        f64 pitch_variation;

        Sound(Audio&& audio, f64 base_pitch, f64 pitch_variation): 
            audio(std::move(audio)), 
            base_pitch(base_pitch), pitch_variation(pitch_variation) {}

        static Sound from_resource(const LoadArgs& args);

        f64 generate_pitch() const;
    };


    struct Listener {
        Vec<3> position = { 0, 0, 0 };
        Vec<3> look_at = { 0, 0, -1 };
        Vec<3> up = { 0, 1, 0 };
        f64 max_speaker_distance = INFINITY;

        void internal_make_current() const;
        static const Vec<3>& internal_position();
        static f64 internal_max_source_dist();
    };

    struct Speaker {
        enum struct Space { World, Listener };

        private:
        static void destruct_source(const u64& source_id);

        util::Handle<u64, &destruct_source> source;

        public:
        Vec<3> position = { 0, 0, 0 };
        Space space = Space::Listener;
        std::shared_ptr<Volume> volume = nullptr;
        f64 gain = 1.0;
        f64 pitch = 1.0;
        
        Vec<3> absolute_position() const {
            return this->space == Space::World? this->position
                : this->position + Listener::internal_position();
        }

        Vec<3> relative_position() const {
            return this->space == Space::Listener? this->position
                : this->position - Listener::internal_position();
        }

        void update(); // must be called every frame

        void play(const Audio& audio);
        void play(const Sound& sound);
        bool is_playing() const;
        void stop();
    };


    struct Soundtrack {

        enum struct Repetition {
            Allowed, Forbidden
        };

        struct LoadArgs {
            using ResourceType = Soundtrack;

            std::vector<std::string_view> track_sources;
            Repetition repetition;
            LoadArgs(
                std::vector<std::string_view>&& track_sources, 
                Repetition repetition = Repetition::Forbidden
            ): track_sources(std::move(track_sources)), repetition(repetition) {}

            std::string identifier() const {  return this->pretty_identifier(); }
            std::string pretty_identifier() const {
                std::string result = "Soundtrack[" 
                    + std::string(this->repetition == Repetition::Allowed
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

        private:
        Speaker speaker;
        std::vector<Audio> tracks;
        Repetition repetition;
        math::StatefulRNG rng;
        u64 last_played_idx = UINT64_MAX;

        public:
        Soundtrack(
            std::vector<Audio> tracks, 
            Repetition repetition = Repetition::Forbidden,
            math::StatefulRNG rng = math::StatefulRNG()
        ): tracks(std::move(tracks)), repetition(repetition), rng(rng) {}

        static Soundtrack from_resource(const LoadArgs& args) {
            std::vector<Audio> tracks;
            tracks.reserve(args.track_sources.size());
            for(std::string_view track_source: args.track_sources) {
                auto track = Audio::from_resource({ track_source });
                tracks.push_back(std::move(track));
            }
            return Soundtrack(std::move(tracks), args.repetition);
        }

        void update() {
            this->speaker.update();
            if(this->speaker.is_playing()) { return; }
            for(;;) {
                u64 track_idx = (u64) (
                    this->rng.next_f64() * (f64) this->tracks.size()
                ) % this->tracks.size();
                bool is_valid = this->repetition == Repetition::Allowed
                    || track_idx != this->last_played_idx;
                if(!is_valid) { continue; }
                this->last_played_idx = track_idx;
                this->speaker.play(this->tracks[track_idx]);
                break;
            }
        }

    };

}