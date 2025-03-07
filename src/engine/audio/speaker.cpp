
#include <engine/audio.hpp>
#include <AL/al.h>

namespace houseofatmos::engine {

    void Speaker::destruct_source(const u64& source_id) {
        ALuint source_gl_id = source_id;
        alDeleteBuffers(1, &source_gl_id);
    }

    static u64 generate_source() {
        ALuint source_id;
        alGenSources(1, &source_id);
        return source_id;
    }

    void Speaker::update() {
        if(Listener::internal_max_source_dist() != INFINITY) {
            f64 distance = this->relative_position().len();
            bool in_range = distance <= Listener::internal_max_source_dist();
            if(in_range && this->source.is_empty()) {
                this->source = util::Handle<u64, &Speaker::destruct_source>(
                    generate_source()
                );
            } 
            if(!in_range && !this->source.is_empty()) {
                this->source = util::Handle<u64, &Speaker::destruct_source>();
            }
        }
        if(!this->source.is_empty()) {
            f64 volume_gain = this->volume == nullptr? 1.0 : this->volume->gain;
            f64 final_gain = this->gain * volume_gain;
            alSourcef(*this->source, AL_GAIN, final_gain);
        }
    }

    void Speaker::play(const Audio& audio) {
        if(this->source.is_empty()) { return; }
        alSourceStop(*this->source);
        alSourcef(*this->source, AL_PITCH, this->pitch);
        alSourcei(*this->source, AL_BUFFER, audio.internal_buffer_id());
        alSourcePlay(*this->source);
    }

    void Speaker::play(const Sound& sound) {
        if(this->source.is_empty()) { return; }
        alSourceStop(*this->source);
        f64 final_pitch = this->pitch * sound.generate_pitch();
        alSourcef(*this->source, AL_PITCH, final_pitch);
        alSourcei(*this->source, AL_BUFFER, sound.audio.internal_buffer_id());
        alSourcePlay(*this->source);
    }

    bool Speaker::is_playing() const {
        if(this->source.is_empty()) { return false; }
        ALint state;
        alGetSourcei(*this->source, AL_SOURCE_STATE, &state);
        return state == AL_PLAYING;
    }

    void Speaker::stop() {
        if(this->source.is_empty()) { return; }
        alSourceStop(*this->source);
    }

}