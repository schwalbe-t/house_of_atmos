
#include <engine/audio.hpp>
#include <engine/logging.hpp>
#include <stb/stb_vorbis.h>
#include <AL/alc.h>
#include <AL/al.h>
#include <cstring>

namespace houseofatmos::engine {

    void Audio::destruct_buffer(const u64& buffer_id) {
        ALuint buffer_al_id = buffer_id;
        alDeleteBuffers(1, &buffer_al_id);
    }

    static std::vector<i16> decode(
        const std::vector<char>& encoded, u8& channels, u64& sample_rate,
        std::string_view path
    ) {
        static_assert(sizeof(i16) == sizeof(short));
        stb_vorbis* decoder = stb_vorbis_open_memory(
            (unsigned char*) encoded.data(), encoded.size(), nullptr, nullptr
        );
        if(decoder == nullptr) {
            error("Unable to decode the file '" + std::string(path) + "' as OGG");
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
        std::vector<char> encoded = GenericLoader::read_bytes(arg.path);
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
        Audio result;
        result.buffer = util::Handle<u64, &Audio::destruct_buffer>(buffer_id);
        result.data_channels = channels;
        result.data_sample_rate = sample_rate;
        return result;
    }

}