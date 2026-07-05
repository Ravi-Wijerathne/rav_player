#pragma once

#include <functional>
#include <memory>

#include "AudioFrame.h"

namespace rav {

struct AudioOutputSpec {
    int sample_rate{48000};
    AVSampleFormat format{AV_SAMPLE_FMT_FLT};
    int channels{2};
    int64_t channel_layout{0};
    int frames_per_buffer{1024};
};

class AudioOutput {
public:
    using FillCallback = std::function<int(uint8_t*, int)>;
    virtual void set_fill_callback(FillCallback) {}

    virtual ~AudioOutput() = default;

    virtual bool init(const AudioOutputSpec& spec) = 0;
    virtual void shutdown() = 0;

    virtual bool play() = 0;
    virtual bool pause() = 0;
    virtual bool resume() = 0;
    virtual bool stop() = 0;

    virtual int write_frames(const uint8_t* data, int frames) = 0;

    virtual double latency() const = 0;
    virtual bool is_playing() const = 0;
    virtual bool is_initialized() const = 0;
};

} // namespace rav
