#pragma once

#include <functional>
#include <memory>

#include "../audio/AudioOutput.h"

#include <AudioToolbox/AudioToolbox.h>

namespace rav {

class MacOSAudioOutput : public AudioOutput {
public:
    MacOSAudioOutput() = default;
    ~MacOSAudioOutput() override { shutdown(); }

    MacOSAudioOutput(const MacOSAudioOutput&) = delete;
    MacOSAudioOutput& operator=(const MacOSAudioOutput&) = delete;

    bool init(const AudioOutputSpec& spec) override;
    void shutdown() override;

    bool play() override;
    bool pause() override;
    bool resume() override;
    bool stop() override;

    int write_frames(const uint8_t* data, int frames) override {
        if (fill_cb_) {
            return fill_cb_(const_cast<uint8_t*>(data), frames);
        }
        return frames;
    }

    double latency() const override { return 0.05; }
    bool is_playing() const override { return playing_; }
    bool is_initialized() const override { return initialized_; }

    using FillCallback = std::function<int(uint8_t*, int)>;
    void set_fill_callback(FillCallback cb) override { fill_cb_ = std::move(cb); }

private:
    static void audio_queue_callback(void* user_data, AudioQueueRef queue,
                                     AudioQueueBufferRef buffer);

    AudioQueueRef queue_{nullptr};
    AudioQueueBufferRef buffers_[3]{};
    AudioStreamBasicDescription asbd_{};
    AudioOutputSpec spec_;
    bool playing_{false};
    bool initialized_{false};
    FillCallback fill_cb_;
};

} // namespace rav


