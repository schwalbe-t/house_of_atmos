
#include <engine/audio.hpp>
#include <AL/al.h>

namespace houseofatmos::engine {

    void Speaker::destruct_source(const u64& source_id) {
        ALuint source_gl_id = source_id;
        alDeleteSources(1, &source_gl_id);
    }

    static u64 generate_source() {
        ALuint source_id;
        alGenSources(1, &source_id);
        return source_id;
    }

    void Speaker::update() {
        f64 max_dist = Listener::internal_max_source_dist();
        bool in_range = max_dist == INFINITY
            || this->relative_position().len() <= max_dist;
        if(in_range && this->source.is_empty()) {
            this->source = util::Handle<u64, &Speaker::destruct_source>(
                generate_source()
            );
        } 
        if(!in_range && !this->source.is_empty()) {
            this->source = util::Handle<u64, &Speaker::destruct_source>();
        }
        if(!this->source.is_empty()) {
            Vec<3> pos = this->absolute_position();
            alSource3f(*this->source, AL_POSITION, pos.x(), pos.y(), pos.z());
            f64 volume_gain = this->volume == nullptr? 1.0 : this->volume->gain;
            f64 final_gain = this->gain * volume_gain;
            alSourcef(*this->source, AL_GAIN, final_gain);
            alSourcef(*this->source, AL_REFERENCE_DISTANCE, this->range);
        }
    }

    void Speaker::play(const Audio& audio) {
        this->update();
        if(this->source.is_empty()) { return; }
        alSourceStop(*this->source);
        alSourcei(*this->source, AL_BUFFER, audio.internal_buffer_id());
        alSourcef(*this->source, AL_PITCH, this->pitch);
        alSourcePlay(*this->source);
    }

    void Speaker::play(const Sound& sound) {
        this->update();
        if(this->source.is_empty()) { return; }
        alSourceStop(*this->source);
        alSourcei(*this->source, AL_BUFFER, sound.audio.internal_buffer_id());
        f64 final_pitch = this->pitch * sound.generate_pitch();
        alSourcef(*this->source, AL_PITCH, final_pitch);
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