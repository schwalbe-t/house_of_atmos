
#include <engine/audio.hpp>
#include <engine/logging.hpp>
#include <stb/stb_vorbis.h>
#include <AL/alc.h>
#include <AL/al.h>
#include <cstring>

namespace houseofatmos::engine {

    Audio::Audio(Audio&& other) noexcept {
        if(other.moved) {
            error("Attempted to move an already moved 'Audio'");
        }
        this->data_channels = other.data_channels;
        this->data_sample_rate = other.data_sample_rate;
        this->buffer_id = other.buffer_id;
        this->source_id = other.source_id;
        this->moved = false;
        other.moved = true;
    }

    static std::vector<i16> decode(
        const std::vector<char>& encoded, u8& channels, u64& sample_rate,
        const std::string& path
    ) {
        static_assert(sizeof(i16) == sizeof(short));
        stb_vorbis* decoder = stb_vorbis_open_memory(
            (unsigned char*) encoded.data(), encoded.size(), nullptr, nullptr
        );
        if(decoder == nullptr) {
            error("Unable to decode the file '" + path + "' as OGG");
        }
        stb_vorbis_info info = stb_vorbis_get_info(decoder);
        channels = info.channels;
        sample_rate = info.sample_rate;
        std::vector<i16> decoded;
        i16 frame[4096];
        for(;;) {
            size_t channel_samples = stb_vorbis_get_frame_short_interleaved(
                decoder, channels, frame, sizeof(frame) / sizeof(i16)
            );
            if(channel_samples == 0) { break; }
            size_t total_samples = channel_samples * channels;
            decoded.insert(decoded.end(), frame, frame + total_samples);
        }
        stb_vorbis_close(decoder);
        return decoded;
    }

    Audio Audio::from_resource(const LoadArgs& arg) {
        std::vector<char> encoded = GenericResource::read_bytes(arg.path);
        u8 channels;
        u64 sample_rate;
        std::vector<i16> decoded = decode(
            encoded, channels, sample_rate, arg.path
        );
        ALuint buffer_id;
        alGenBuffers(1, &buffer_id);
        alBufferData(
            buffer_id, 
            channels == 1? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, 
            (const ALvoid*) decoded.data(), 
            decoded.size() * sizeof(i16), 
            sample_rate
        );
        ALuint source_id;
        alGenSources(1, &source_id);
        alSourcei(source_id, AL_BUFFER, buffer_id);
        Audio result;
        result.data_channels = channels;
        result.data_sample_rate = sample_rate;
        result.buffer_id = buffer_id;
        result.source_id = source_id;
        result.moved = false;
        return result;
    }

    Audio& Audio::operator=(Audio&& other) noexcept {
        if(this == &other) { return *this; }
        if(other.moved) {
            error("Attempted to move an already moved 'Audio'");
        }
        if(!this->moved) {
            ALuint buffer_id = this->buffer_id;
            alDeleteBuffers(1, &buffer_id);
            ALuint source_id = this->source_id;
            alDeleteBuffers(1, &source_id);
        }
        this->data_channels = other.data_channels;
        this->data_sample_rate = other.data_sample_rate;
        this->buffer_id = other.buffer_id;
        this->source_id = other.source_id;
        this->moved = false;
        other.moved = true;
        return *this;
    }

    Audio::~Audio() {
        if(this->moved) { return; }
        ALuint buffer_id = this->buffer_id;
        alDeleteBuffers(1, &buffer_id);
        ALuint source_id = this->source_id;
        alDeleteBuffers(1, &source_id);
    }

    void Audio::play() const {
        alSourcePlay(this->source_id);
    }

    void Audio::stop() const {
        alSourceStop(this->source_id);
    }

    void Audio::set_pitch(f64 value) const {
        alSourcef(this->source_id, AL_PITCH, value);
    }

    void Audio::set_gain(f64 value) const {
        alSourcef(this->source_id, AL_GAIN, value);
    }

    bool Audio::is_playing() const {
        ALint state;
        alGetSourcei(this->source_id, AL_SOURCE_STATE, &state);
        return state == AL_PLAYING;
    }

}