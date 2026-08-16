#pragma once

#include <functional>
#include <memory>
#include <AudioToolbox/AudioToolbox.h>

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

    AudioOutput() = default;
    ~AudioOutput() { shutdown(); }

    AudioOutput(const AudioOutput&) = delete;
    AudioOutput& operator=(const AudioOutput&) = delete;

    bool init(const AudioOutputSpec& spec);
    void shutdown();

    bool play();
    bool pause();
    bool resume();
    bool stop();

    int write_frames(const uint8_t* data, int frames) {
        if (fill_cb_) {
            return fill_cb_(const_cast<uint8_t*>(data), frames);
        }
        return frames;
    }

    double latency() const { return 0.05; }
    bool is_playing() const { return playing_; }
    bool is_initialized() const { return initialized_; }

    void set_fill_callback(FillCallback cb) { fill_cb_ = std::move(cb); }

    void set_volume(float vol);

private:
    static void audio_queue_callback(void* user_data, AudioQueueRef queue,
                                     AudioQueueBufferRef buffer);

    AudioQueueRef queue_{nullptr};
    AudioQueueBufferRef buffers_[3]{};
    AudioStreamBasicDescription asbd_{};
    AudioOutputSpec spec_;
    bool playing_{false};
    bool initialized_{false};
    float volume_{1.0f};
    FillCallback fill_cb_;
};

} // namespace rav
