#pragma once

#include <atomic>
#include <chrono>
#include <cmath>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "../decoder/PacketQueue.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libavutil/display.h>
}

#include "PlayerState.h"
#include "PlayerCommand.h"
#include "PlayerEvent.h"
#include "EventBus.h"
#include "../ffmpeg/ContainerReader.h"
#include "../ffmpeg/FFmpegContext.h"
#include "../decoder/VideoDecoder.h"
#include "../decoder/AudioDecoder.h"
#include "../video/VideoFrame.h"
#include "../video/FrameQueue.h"
#include "../video/VideoClock.h"
#include "../audio/AudioFrame.h"
#include "../audio/AudioQueue.h"
#include "../audio/AudioClock.h"
#include "../audio/AudioOutput.h"
#include "../rendering/MetalRenderer.h"
#include "../decoder/SubtitleDecoder.h"
#include "../subtitles/SubtitleQueue.h"

namespace rav {

class PlaybackEngine {
public:
    PlaybackEngine();
    ~PlaybackEngine();

    PlaybackEngine(const PlaybackEngine&) = delete;
    PlaybackEngine& operator=(const PlaybackEngine&) = delete;

    bool open(const std::string& url);
    void close();
    void play();
    void pause();
    void seek(double seconds);
    void stop();

    PlayerState state() const { return state_.load(); }
    double duration() const { return duration_; }

    double current_time() const {
        if (audio_clock_.is_running()) {
            return audio_clock_.pts();
        }
        return video_clock_.pts();
    }

    bool has_subtitles() const { return subtitle_stream_ >= 0; }

    std::vector<SubtitleFrame> current_subtitles() {
        return subtitle_queue_.subtitles_at_time(current_time());
    }

    void set_video_renderer(MetalRenderer* renderer) { video_renderer_ = renderer; }

    EventBus<PlayerCommand>& command_bus() { return command_bus_; }
    EventBus<PlayerEvent>& event_bus() { return event_bus_; }

    void set_volume(float vol) { 
        volume_ = vol;
        if (audio_output_) audio_output_->set_volume(vol);
    }
    float volume() const { return volume_; }

    int fill_audio_buffer(uint8_t* data, int frames_requested);
    bool has_video() const { return video_stream_ >= 0; }
    bool has_audio() const { return audio_stream_ >= 0; }
    int video_queue_size() const { return video_queue_.size(); }
    int audio_queue_size() const { return audio_queue_.size(); }

    bool sync_pop_video_frame(VideoFrame& out);
    VideoFrame pop_video_frame() { return video_queue_.pop(); }
    bool try_pop_video_frame(VideoFrame& out, int timeout_ms = 0) { return video_queue_.try_pop(out, timeout_ms); }

    // Metadata accessors
    std::string media_title() const { return media_title_; }
    std::string media_artist() const { return media_artist_; }
    std::string media_album() const { return media_album_; }
    int media_bitrate() const { return media_bitrate_; }
    std::string video_codec_name();
    std::string audio_codec_name();

    int video_width() const;
    int video_height() const;

private:
    void set_state(PlayerState new_state);
    void resume_internal();
    void flush_buffers();
    void handle_seek();
    void demux_thread_fn();
    void video_decode_thread_fn();
    void audio_decode_thread_fn();
    bool resample_audio(const AVFrame* frame, std::vector<uint8_t>& out_buffer);
    int get_video_rotation();

    EventBus<PlayerCommand> command_bus_;
    EventBus<PlayerEvent> event_bus_;

    ContainerReader reader_;
    VideoDecoder video_decoder_;
    AudioDecoder audio_decoder_;
    SubtitleDecoder subtitle_decoder_;

    FrameQueue video_queue_{30};
    AudioQueue audio_queue_{30};
    SubtitleQueue subtitle_queue_;

    PacketQueue video_packet_queue_{120};
    PacketQueue audio_packet_queue_{120};

    AudioFrame current_audio_frame_;
    int leftover_audio_frames_{0};
    int leftover_audio_offset_{0};

    VideoClock video_clock_;
    AudioClock audio_clock_;

    MetalRenderer* video_renderer_{nullptr};
    std::unique_ptr<AudioOutput> audio_output_;

    int video_stream_{-1};
    int audio_stream_{-1};
    int subtitle_stream_{-1};

    int video_width_{0};
    int video_height_{0};
    double duration_{0.0};
    std::atomic<PlayerState> state_{PlayerState::Idle};
    std::atomic<float> volume_{1.0f};

    std::mutex control_mutex_;
    std::thread playback_thread_;
    std::thread video_decode_thread_;
    std::thread audio_decode_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> seek_requested_{false};
    std::atomic<bool> flush_requested_{false};
    bool audio_started_{false};
    bool video_clock_locked_{false};
    double seek_target_{0.0};

    SwrContext* swr_ctx_{nullptr};
    AVSampleFormat last_in_sample_fmt_{AV_SAMPLE_FMT_NONE};
    int last_in_sample_rate_{0};
    AVChannelLayout last_in_ch_layout_{};
    int video_rotation_{0};

    std::string media_title_;
    std::string media_artist_;
    std::string media_album_;
    int media_bitrate_{0};
};

} // namespace rav
