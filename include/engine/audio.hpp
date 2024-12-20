
#pragma once

#include "scene.hpp"
#include "nums.hpp"
#include <vector>

namespace houseofatmos::engine {

    struct Audio {
        struct LoadArgs {
            std::string path;

            std::string identifier() const { return path; }
            std::string pretty_identifier() const {
                return "Audio@'" + path + "'";
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

        void play() const;
        void stop() const;
        void set_pitch(f64 value) const;
        void set_gain(f64 value) const;
        bool is_playing() const;
    };

}